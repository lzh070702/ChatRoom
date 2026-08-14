#include <csignal>
#include <glog/logging.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pool.h"
#include "reactor/Reactor.h"
#include "net/TcpServer.h"
#include "net/Connection.h"


using namespace std;

const int SUB_CNT = 4;

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    signal(SIGPIPE, SIG_IGN);
    mkdir("./files", 0755);
    pool thread_pool(SUB_CNT);
    vector<unique_ptr<Reactor>> subs(SUB_CNT);
    for (int i = 0; i < SUB_CNT; i++) {
        subs[i] = make_unique<Reactor>();
        thread_pool.enqueue([reactor = subs[i].get()]() {
            reactor->loop();
        });
    }
    TcpServer server(8000);
    if (!server.start()) {
        LOG(ERROR) << "Server start failed";
        return -1;
    }
    LOG(INFO) << "Server started on port 8000";
    int main_epfd = epoll_create1(0);
    epoll_event ev{}, events[1024];
    ev.events = EPOLLIN;
    ev.data.fd = server.getLfd();
    epoll_ctl(main_epfd, EPOLL_CTL_ADD, server.getLfd(), &ev);
    while (true) {
        int n = epoll_wait(main_epfd, events, 1024, -1);
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == server.getLfd()) {
                while (true) {
                    int cfd = server.acceptCli();
                    if (cfd < 0) {
                        break;
                    }
                    int min_idx{0};
                    for (int j = 0; j < SUB_CNT; j++) {
                        if (subs[j]->getConnCnt() <
                            subs[min_idx]->getConnCnt()) {
                            min_idx = j;
                        }
                    }
                    subs[min_idx]->pushFd(cfd);
                }
            }
        }
    }
    server.closeLfd();
    close(main_epfd);
    return 0;
}
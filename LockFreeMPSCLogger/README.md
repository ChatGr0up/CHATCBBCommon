1、测试程序
    const int num_threads = 100;
    const int logs_per_thread = 100000;

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, logs_per_thread]() {
            for (int i = 0; i < logs_per_thread; ++i) {
                LOG::LockFreeMPSCLogger::instance().log(LOG::LogLevel::INFO, std::to_string(t) + ", CHATProcessor started." + " Log #" + std::to_string(i));
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end_time - start_time;

    std::cout << "Logged " << num_threads * logs_per_thread
              << " messages in " << diff.count() << " seconds." << std::endl;
2、一千万条日志需要8s
3、一百万条日志需要1.2s
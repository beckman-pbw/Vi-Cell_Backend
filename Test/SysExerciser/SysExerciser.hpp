#pragma once

#include <memory>

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include "HawkeyeThread.hpp"

class SysExerciser
{
public:
	class LoadCPU {
	public:
		LoadCPU() {
			//int xxx = 0;
		}
		~LoadCPU() {
		}

		void Start (size_t threadNum) {
			std::cout << "Thread " << threadNum << " started..." << std::endl;
			isRunning_ = true;
			
			// Continually compute all of the prime numbers withing uint64_t space.
			while (isRunning_) {

				// Compute all prime numbers.
				for (uint64_t number_test = 2; number_test; number_test++) {
					if (!isRunning_) {
						break;
					}
					for (uint64_t divisor = 2; divisor < number_test; divisor++) {
						if (!isRunning_) {
							break;
						}
						if (number_test % divisor) {
							//std::cout << number_test << " ";
						}
						break;
					}
				}
			}

			std::cout << "Thread " << threadNum << " stopped..." << std::endl;
		}

		void Stop() {
			isRunning_ = false;
		}

	private:
		bool isRunning_;
	};


//TODO:
// For RAM usage,
//   determine 
// check for disk space approaching allowed maximum of 475GB (the warning threshold is 5% of disk capacity,
// so for a 500 GB disk 25 GB is the reserve so 475 GB is available for data)

#define MBYTE (1024 * 1024 * 100)
	class MByte {
	public:
		MByte() {

		}
		~MByte() {

		}

	private:
		char data[MBYTE];
	};


	class LoadRAM {
	public:
		LoadRAM() {
			int xxx = 0;
			//std::cout << "LoadRAM()" << std::endl;
		}
		~LoadRAM() {
		}

		void Decrease() {
			if (data.size()) {
				MByte* ptr = data.back();
				free (ptr);
				data.pop_back();
			} else {
				std::cout << "No RAM allocated..." << std::endl;
			}
		}

		void Increase() {
			data.push_back (new MByte);
		}

	private:
		uint64_t memorySize;
		std::vector<MByte*> data;
	};


	SysExerciser (std::shared_ptr<boost::asio::io_context> pIoSvc);
	virtual ~SysExerciser();
	bool Start();

private:
	void prompt();
	void showHelp();
	void handleInput (const boost::system::error_code& ec);

	std::shared_ptr<boost::asio::io_context> pIoSvc_;

	std::vector<std::unique_ptr<HawkeyeThread>> pCPUThreads_;
	std::vector<std::shared_ptr<SysExerciser::LoadCPU>> pLoadCPU_;

	std::shared_ptr<SysExerciser::LoadRAM> pLoadRAM_;
	std::unique_ptr<HawkeyeThread> pRAMThread_;
};

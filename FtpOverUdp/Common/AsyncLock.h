#ifdef COMMON_EXPORTS
#define COMMON_API __declspec(dllexport)
#else
#define COMMON_API __declspec(dllimport)
#endif

#include <mutex>
#include <condition_variable>

namespace Common
{
	class COMMON_API AsyncLock
	{
	private:
		bool isAsyncReady;
		std::mutex readLock;
		std::condition_variable readOperation;

		bool isAsyncDone;
		std::mutex writeLock;
		std::condition_variable writeOperation;
	public:
		AsyncLock(bool waitForReader, bool waitForWriter);

		void waitForConsumption();
		void finalizeConsumption();

		void waitForSignalling();
		void finalizeSignalling();
	};
}
#include <occa/utils/mutex.hpp>
#include <occa/utils/logging.hpp>

namespace occa {
  mutex_t::mutex_t() 
#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
    :
    mutexHandle(PTHREAD_MUTEX_INITIALIZER) {
    int error = pthread_mutex_init(&mutexHandle, NULL);
#if OCCA_UNSAFE
    ignoreResult(error);
#endif

    OCCA_ERROR("Error initializing mutex",
               error == 0);
#else
    {
#endif
  }

  void mutex_t::free() {
#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
    int error = pthread_mutex_destroy(&mutexHandle);
#if OCCA_UNSAFE
    ignoreResult(error);
#endif

    OCCA_ERROR("Error freeing mutex",
               error == 0);

#endif
  }

  void mutex_t::lock() {
#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
    pthread_mutex_lock(&mutexHandle);
#else
    mutexHandle.lock();
#endif
  }

  void mutex_t::unlock() {
#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_MACOS_OS))
    pthread_mutex_unlock(&mutexHandle);
#else
    mutexHandle.unlock();
#endif
  }
}

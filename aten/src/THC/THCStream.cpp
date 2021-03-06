#include "THCStream.hpp"

#include <mutex>
#include <cuda_runtime_api.h>

#define MAX_DEVICES 256
static THCStream default_streams[MAX_DEVICES];

static void initialize_default_streams()
{
  for (int i = 0; i < MAX_DEVICES; i++) {
    default_streams[i].device = i;
  }
}

THCStream* THCStream_new(int flags)
{
  THCStream* self = (THCStream*) malloc(sizeof(THCStream));
  self->refcount = 1;
  THCudaCheck(cudaGetDevice(&self->device));
  THCudaCheck(cudaStreamCreateWithFlags(&self->stream, flags));
  return self;
}

THC_API THCStream* THCStream_defaultStream(int device)
{
  // default streams aren't refcounted
  THAssert(device >= 0 && device < MAX_DEVICES);
  std::once_flag once;
  std::call_once(once, &initialize_default_streams);
  return &default_streams[device];
}

THC_API cudaStream_t THCStream_stream(THCStream* self) { return self->stream; }
THC_API int THCStream_device(THCStream* self) { return self->device; }

THCStream* THCStream_newWithPriority(int flags, int priority)
{
  THCStream* self = (THCStream*) malloc(sizeof(THCStream));
  self->refcount = 1;
  THCudaCheck(cudaGetDevice(&self->device));
  THCudaCheck(cudaStreamCreateWithPriority(&self->stream, flags, priority));
  return self;
}

void THCStream_free(THCStream* self)
{
  if (!self || !self->stream) {
    return;
  }
  if (--self->refcount == 0) {
    THCudaCheckWarn(cudaStreamDestroy(self->stream));
    free(self);
  }
}

void THCStream_retain(THCStream* self)
{
  if (self->stream) {
    self->refcount++;
  }
}

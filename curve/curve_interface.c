#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "curve_interface.h"


to_curve xy_to_curve = NULL;
from_curve curve_to_xy = NULL;
name_of_curve curve_name = NULL;
version_of_curve curve_version = NULL;

void load_curve(const char * sofilename) {
  char * message;
  void * handle;

  handle = dlopen(sofilename,  RTLD_NOW); // NULL implies failure
  if (handle == NULL) {
    fprintf(stderr, "%s\n", dlerror());
    exit(-1);
  }

  xy_to_curve = (to_curve)dlsym(handle, "xy_to_curve"); // NULL does not imply failure
  if ((message = dlerror()) != NULL) {
    fprintf(stderr, "%s\n", message);
    exit(-1);
  }

  curve_to_xy = (from_curve)dlsym(handle, "curve_to_xy"); // NULL does not imply failure
  if ((message = dlerror()) != NULL) {
    fprintf(stderr, "%s\n", message);
    exit(-1);
  }

  curve_name = (name_of_curve)dlsym(handle, "curve_name"); //NULL does not imply failure
  if ((message = dlerror()) != NULL) {
    fprintf(stderr, "%s\n", message);
    exit(-1);
  }

  curve_version = (version_of_curve)dlsym(handle, "curve_version"); //NULL does not imply failure
  if ((message = dlerror()) != NULL) {
    fprintf(stderr, "%s\n", message);
    exit(-1);
  }
}

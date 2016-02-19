#ifndef NMR_COMMON_INCLUDE_MIDAS_HELPER_FUNCTIONS_HH_
#define NMR_COMMON_INCLUDE_MIDAS_HELPER_FUNCTIONS_HH_

/*====================================================================*\

  file: midas_helper_functions.hh
  author: Matthias W. Smith
  
  about: Implements several wrappers of midas functionality such as
         finding an item in the experiment's odb and returing as
	 a string.

\*====================================================================*/

//--- std includes ---------------------------------------------------//
#include <string>

//--- other includes -------------------------------------------------//
#include "midas.h"

//--- project includes -----------------------------------------------//

template <typename T>
inline T odb_get(const std::string &key, T val, int type=TID_STRING) {
  int size = 1024;
  char str[1024];
  HNDLE hDB, hkey;
  T ret;
    
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, key.c_str(), &hkey);
  
  if (hkey && type==TID_STRING) {

    db_get_data(hDB, hkey, &str, &size, type);
    ret = std::string(str);

  } else {
    
    size = sizeof(ret);
    db_get_data(hDB, hkey, &ret, &size, type);

  }

  return ret;
};

#endif

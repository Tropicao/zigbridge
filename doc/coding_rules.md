# Coding rules
This documents aims to centralize coding rules used in this project. Please do not hesitate to add any missing elements.

* The global project prefix is zg (for ZLL Gateway)
* Exported functions (i.e accessible by including proper header), should be named as follow :
  ```project_module_function```  

Eg: ```zg_mt_af_register_zll_callback```
* Each header file should be protected against recursive inclusion. The checked variable should use the following syntax :
    ```#ifdef PREFIX_MODULE_H```

Eg : ```#ifdef ZG_ZLL_H```

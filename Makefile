CXX = g++

LIB_DIR = /home/aloiselkb/sis3315_implementation/sis3315-software/libraries_and_includes

INCLUDES = \
    -I$(LIB_DIR)/sis_vme_master_class_lib \
    -I$(LIB_DIR)/sis3315_class_library \
    -I$(LIB_DIR)/sis3315_header

SRCS_LIB = $(LIB_DIR)/sis_vme_master_class_lib/sis3315_ethernet_access_class.cpp

unit_test/%: unit_test/%.cpp
	$(CXX) $< $(SRCS_LIB) $(INCLUDES) -o $@ && ./$@

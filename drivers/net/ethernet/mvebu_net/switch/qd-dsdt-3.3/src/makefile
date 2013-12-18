# makefile - build main object file
#
# modification history
# --------------------
# 04-15-02,mj	created
#
#######################################################################

TARGET = $(DSDT_PROJ_NAME)

ifeq ($(OS_RUN),VXWORKS)
WORK_TO_DO = $(DSDT_PROJ_NAME).o

# Include common variable definitions
include $(DSDT_TOOL_DIR)\make.defs
endif

ifeq ($(OS_RUN),LINUX)
WORK_TO_DO = $(DSDT_PROJ_NAME).o

# Include common variable definitions
include $(DSDT_TOOL_DIR)/makelnx.defs
endif

exe : $(WORK_TO_DO)

.PHONY : exe

vpath %.o    $(OBJDIR)
vpath %.lib  $(OBJDIR)

MAINDRV_PATH = $(SRC_BASE_PATH)
MAD_ROOT = $(SRC_BASE_PATH)/../../phy

SRC_OBJ_DIR = $(MAINDRV_PATH)/driver \
               $(MAINDRV_PATH)/msapi \
               $(MAINDRV_PATH)/platform

OBJECTS_LIST = $(MAINDRV_PATH)/driver/$(OBJDIR)/driver.o \
               $(MAINDRV_PATH)/msapi/$(OBJDIR)/msapi.o \
               $(MAINDRV_PATH)/platform/$(OBJDIR)/platform.o

OBJECTS = $(OBJECTS_LIST)

OBJDIRS	= $(subst /,\,$(dir $(OBJECTS)))
LIBDIRS	= $(subst /,\,$(LIB_DIR))

OBJDIRS_/ = $(subst \,/,$(OBJDIRS))
LIBDIRS_/ = $(subst \,/,$(LIBDIRS))

ifeq ($(OS_RUN),VXWORKS)
$(DSDT_PROJ_NAME).o : $(OBJECTS) $(MAD_OBJ)
	@ $(ECHO) '------------'
	@ $(ECHO) 'Building $@'
	$(LD) $(LDFLAGS) -Map $(LIB_DIR)/$(DSDT_PROJ_NAME).map -o $(LIB_DIR)/$(DSDT_PROJ_NAME).o $(OBJECTS) $(MAD_OBJ)

$(OBJECTS) : FORCE
	@($(CD) $(subst /,\,$(dir $(@D)))) && $(MAKE)

$(MAD_OBJ) : FORCE
	$(MAKE) -C $(MAD_ROOT)/src

endif

ifeq ($(OS_RUN),LINUX)
$(DSDT_PROJ_NAME).o : $(OBJECTS) $(MAD_OBJ)
	@ $(ECHO) '------------'
	@ $(ECHO) 'Building $@'
	$(LD) $(LDFLAGS) -Map $(LIB_DIR)/$(DSDT_PROJ_NAME).map -o $(LIB_DIR)/$(DSDT_PROJ_NAME).o $(OBJECTS) $(MAD_OBJ)

$(OBJECTS) : FORCE
	@cd $(dir $(@D)); $(MAKE)

$(MAD_OBJ) : FORCE
	$(MAKE) -C $(MAD_ROOT)/src

endif

FORCE :

.PHONY : clean
ifeq ($(OS_RUN),VXWORKS)
clean :
	@for %x in ($(OBJDIRS)) do \
	    @($(CD) %x..) && $(MAKE) clean
	- ($(CD) $(LIBDIRS)) && $(RM) $(DSDT_PROJ_NAME).o
	- ($(CD) $(LIBDIRS)) && $(RM) $(DSDT_PROJ_NAME).map
	@if exist $(MAD_ROOT) $(MAKE) -C $(MAD_ROOT)/src clean
endif

ifeq ($(OS_RUN),LINUX)
clean :
	@for i in $(SRC_OBJ_DIR); do	\
		cd $$i; $(RM) -r -f *.o *.map makedeps *obj; 	\
	done
#		cd $$i; $(RM) -f *.o *.map;
	@for i in $(OBJDIRS_/); do	\
		$(RM) -r -f  $$i;	\
	done
#		cd $$i..; $(MAKE) clean;
	@cd $(LIB_DIR); $(RM) *.o
	@cd $(LIB_DIR); $(RM) *.map
	$(MAKE) -C $(MAD_ROOT)/src clean
endif

#end of file

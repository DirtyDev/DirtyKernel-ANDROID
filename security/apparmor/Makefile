# Makefile for AppArmor Linux Security Module
#
obj-$(CONFIG_SECURITY_APPARMOR) += apparmor.o

apparmor-y := apparmorfs.o audit.o capability.o context.o ipc.o lib.o match.o \
              path.o domain.o policy.o policy_unpack.o procattr.o lsm.o \
              resource.o sid.o file.o net.o

apparmor-$(CONFIG_SECURITY_APPARMOR_COMPAT_24) += apparmorfs-24.o

clean-files := capability_names.h rlim_names.h af_names.h


# Build a lower case string table of capability names
# Transforms lines from
#    #define CAP_DAC_OVERRIDE     1
# to
#    [1] = "dac_override",
quiet_cmd_make-caps = GEN     $@
cmd_make-caps = echo "static const char *capability_names[] = {" > $@ ;\
	sed $< >>$@ -r -n -e '/CAP_FS_MASK/d' \
	-e 's/^\#define[ \t]+CAP_([A-Z0-9_]+)[ \t]+([0-9]+)/[\2] = "\L\1",/p';\
	echo "};" >> $@

quiet_cmd_make-af = GEN     $@
cmd_make-af = echo "static const char *address_family_names[] = {" > $@ ; sed -n -e "/AF_MAX/d" -e "/AF_LOCAL/d" -e "s/^\#define[ \\t]\\+AF_\\([A-Z0-9_]\\+\\)[ \\t]\\+\\([0-9]\\+\\)\\(.*\\)\$$/[\\2]  = \"\\1\",/p" $< | tr A-Z a-z >> $@ ; echo "};" >> $@


# Build a lower case string table of rlimit names.
# Transforms lines from
#    #define RLIMIT_STACK		3	/* max stack size */
# to
#    [RLIMIT_STACK] = "stack",
#
# and build a second integer table (with the second sed cmd), that maps
# RLIMIT defines to the order defined in asm-generic/resource.h  Thi is
# required by policy load to map policy ordering of RLIMITs to internal
# ordering for architectures that redefine an RLIMIT.
# Transforms lines from
#    #define RLIMIT_STACK		3	/* max stack size */
# to
# RLIMIT_STACK, 
quiet_cmd_make-rlim = GEN     $@
cmd_make-rlim = echo "static const char *rlim_names[] = {" > $@ ;\
	sed $< >> $@ -r -n \
	    -e 's/^\# ?define[ \t]+(RLIMIT_([A-Z0-9_]+)).*/[\1] = "\L\2",/p';\
	echo "};" >> $@ ;\
	echo "static const int rlim_map[] = {" >> $@ ;\
	sed -r -n "s/^\# ?define[ \t]+(RLIMIT_[A-Z0-9_]+).*/\1,/p" $< >> $@ ;\
	echo "};" >> $@

$(obj)/capability.o : $(obj)/capability_names.h
$(obj)/net.o : $(obj)/af_names.h
$(obj)/resource.o : $(obj)/rlim_names.h
$(obj)/capability_names.h : $(srctree)/include/linux/capability.h
	$(call cmd,make-caps)
$(obj)/rlim_names.h : $(srctree)/include/asm-generic/resource.h
	$(call cmd,make-rlim)
$(obj)/af_names.h : $(srctree)/include/linux/socket.h
	$(call cmd,make-af)

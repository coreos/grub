#! /usr/bin/python

#
# This is the python script used to generate Makefile.tpl
#

GRUB_PLATFORMS = [ "emu", "i386_pc", "i386_efi", "i386_qemu", "i386_coreboot",
                   "i386_multiboot", "i386_ieee1275", "x86_64_efi",
                   "mips_yeeloong", "sparc64_ieee1275",
                   "powerpc_ieee1275" ]

GROUPS = {}

GROUPS["common"]   = GRUB_PLATFORMS[:]

# Groups based on CPU
GROUPS["i386"]     = [ "i386_pc", "i386_efi", "i386_qemu", "i386_coreboot", "i386_multiboot", "i386_ieee1275" ]
GROUPS["x86_64"]   = [ "x86_64_efi" ]
GROUPS["x86"]      = GROUPS["i386"] + GROUPS["x86_64"]
GROUPS["mips"]     = [ "mips_yeeloong" ]
GROUPS["sparc64"]  = [ "sparc64_ieee1275" ]
GROUPS["powerpc"]  = [ "powerpc_ieee1275" ]

# Groups based on firmware
GROUPS["x86_efi"]  = [ "i386_efi", "x86_64_efi" ]
GROUPS["ieee1275"]   = [ "i386_ieee1275", "sparc64_ieee1275", "powerpc_ieee1275" ]

# emu is a special case so many core functionality isn't needed on this platform
GROUPS["noemu"]   = GRUB_PLATFORMS[:]; GROUPS["noemu"].remove("emu")

# Groups based on hardware features
GROUPS["cmos"] = GROUPS["x86"][:] + ["mips_yeeloong"]; GROUPS["cmos"].remove("i386_efi"); GROUPS["cmos"].remove("x86_64_efi")
GROUPS["pci"]      = GROUPS["x86"] + GROUPS["mips"]
GROUPS["usb"]      = GROUPS["pci"]

# If gfxterm is main output console integrate it into kernel
GROUPS["videoinkernel"] = ["mips_yeeloong"]
GROUPS["videomodules"]   = GRUB_PLATFORMS[:];
for i in GROUPS["videoinkernel"]: GROUPS["videomodules"].remove(i)

# Miscelaneous groups schedulded to disappear in future
GROUPS["nosparc64"] = GRUB_PLATFORMS[:]; GROUPS["nosparc64"].remove("sparc64_ieee1275")
GROUPS["i386_coreboot_multiboot_qemu"] = ["i386_coreboot", "i386_multiboot", "i386_qemu"]
GROUPS["nopc"] = GRUB_PLATFORMS[:]; GROUPS["nopc"].remove("i386_pc")

#
# Create platform => groups reverse map, where groups covering that
# platform are ordered by their sizes
#
RMAP = {}
for platform in GRUB_PLATFORMS:
    # initialize with platform itself as a group
    RMAP[platform] = [ platform ]

    for k in GROUPS.keys():
        v = GROUPS[k]
        # skip groups that don't cover this platform
        if platform not in v: continue

        bigger = []
        smaller = []
        # partition currently known groups based on their size
        for group in RMAP[platform]:
            if group in GRUB_PLATFORMS: smaller.append(group)
            elif len(GROUPS[group]) < len(v): smaller.append(group)
            else: bigger.append(group)
        # insert in the middle
        RMAP[platform] = smaller + [ k ] + bigger

#
# Global variables
#
GVARS = []

def gvar_add(var, value):
    if var not in GVARS:
        GVARS.append(var)
    return var + " += " + value + "\n"

def global_variable_initializers():
    r = ""
    for var in GVARS:
        r += var + " ?= \n"
    return r

#
# Per PROGRAM/SCRIPT variables 
#

def var_set(var, value):
    return var + "  = " + value + "\n"

def var_add(var, value):
    return var + " += " + value + "\n"

#
# Autogen constructs
#

def set_canonical_name_suffix(suffix): return "[+ % name `export cname=$(echo %s" + suffix + " | sed -e 's/[^0-9A-Za-z@_]/_/g')` +]"
def cname(): return "[+ % name `echo $cname` +]"

def rule(target, source, cmd):
    if cmd[0] == "\n":
        return "\n" + target + ": " + source + cmd.replace("\n", "\n\t") + "\n"
    else:
        return "\n" + target + ": " + source + "\n\t" + cmd.replace("\n", "\n\t") + "\n"

#
# Template for keys with platform names as values, for example:
#
# kernel = {
#   nostrip = emu;
#   ...
# }
#
def if_platform_tagged(platform, tag, snippet_if, snippet_else=None):
    r = ""
    r += "[+ IF " + tag + " defined +]"
    r += "[+ FOR " + tag + " +][+ CASE " + tag + " +]"
    for group in RMAP[platform]:
        r += "[+ = \"" + group + "\" +]" + snippet_if

    if snippet_else != None: r += "[+ * +]" + snippet_else
    r += "[+ ESAC +][+ ENDFOR +]"

    if snippet_else == None:
        r += "[+ ENDIF +]"
        return r

    r += "[+ ELSE +]" + snippet_else + "[+ ENDIF +]"
    return r

#
# Template for tagged values
#
# module = {
#   extra_dist = ...
#   extra_dist = ...
#   ...
# };
#
def foreach_value(tag, closure):
    return "[+ FOR " + tag + " +]" + closure("[+ ." + tag + " +]") + "[+ ENDFOR +]"

#
# Template for handling best matched values for a platform, for example:
#
# module = {
#   cflags = '-Wall';
#   emu_cflags = '-Wall -DGRUB_EMU=1';
#   ...
# }
#
def foreach_platform_specific_value(platform, suffix, nonetag, closure):
    r = ""
    for group in RMAP[platform]:
        gtag = group + suffix

        if group == RMAP[platform][0]:
            r += "[+ IF " + gtag + " +]"
        else:
            r += "[+ ELIF " + gtag + " +]"

        r += "[+ FOR " + gtag + " +]" + closure("[+ ." + gtag + " +]") + "[+ ENDFOR +]"
    r += "[+ ELSE +][+ FOR " + nonetag + " +]" + closure("[+ ." + nonetag + " +]") + "[+ ENDFOR +][+ ENDIF +]"
    return r

#
# Template for handling values from sum of all groups for a platform,
# for example:
#
# module = {
#   common = kern/misc.c;
#   emu = kern/emu/misc.c;
#   ...
# }
#
def foreach_platform_value (platform, suffix, closure):
    r = ""
    for group in RMAP[platform]:
        gtag = group + suffix

        r += "[+ IF " + gtag + " +]"
        r += "[+ FOR " + gtag + " +]" + closure("[+ ." + gtag + " +]") + "[+ ENDFOR +]"
        r += "[+ ENDIF +]"
    return r

#
# Template for gaurding with platform specific "enable" keys, for example:
#
#  module = {
#    name = pci;
#    noemu = bus/pci.c;
#    emu = bus/emu/pci.c;
#    emu = commands/lspci.c;
#
#    enable = emu;
#    enable = i386_pc;
#    enable = x86_efi;
#    enable = i386_ieee1275;
#    enable = i386_coreboot;
#  };
#
def foreach_enabled_platform(closure):
    r = "[+ IF - enable undefined +]"
    for platform in GRUB_PLATFORMS:
        r += "\nif COND_" + platform + "\n" + closure(platform) + "endif\n"
    r += "[+ ELSE +]"
    for platform in GRUB_PLATFORMS:
        x = "\nif COND_" + platform + "\n" + closure(platform) + "endif\n"
        r += if_platform_tagged(platform, "enable", x)
    r += "[+ ENDIF +]"
    return r

#
# Template for gaurding with platform specific automake conditionals,
# for example:
#
#  module = {
#    name = usb;
#    common = bus/usb/usb.c;
#    noemu = bus/usb/usbtrans.c;
#    noemu = bus/usb/usbhub.c;
#    enable = emu;
#    enable = i386;
#    enable = mips_yeeloong;
#    emu_condition = COND_GRUB_EMU_USB;
#  };
#
def under_platform_specific_conditionals(platform, snippet):
    r  = foreach_platform_specific_value(platform, "_condition", "condition", lambda cond: "if " + cond + "\n")
    r += snippet
    r += foreach_platform_specific_value(platform, "_condition", "condition", lambda cond: "endif " + cond + "\n")
    return r

def platform_specific_values(platform, suffix, nonetag):
    return foreach_platform_specific_value(platform, suffix, nonetag,
                                           lambda value: value + " ")

def platform_values(platform, suffix):
    return foreach_platform_value(platform, suffix, lambda value: value + " ")

def extra_dist():
    return foreach_value("extra_dist", lambda value: value + " ")

def platform_sources(p): return platform_values(p, "")
def platform_nodist_sources(p): return platform_values(p, "_nodist")
def platform_dependencies(p): return platform_values(p, "dependencies", "_dependencies")

def platform_startup(p): return platform_specific_values(p, "_startup", "startup")
def platform_ldadd(p): return platform_specific_values(p, "_ldadd", "ldadd")
def platform_cflags(p): return platform_specific_values(p, "_cflags", "cflags")
def platform_ldflags(p): return platform_specific_values(p, "_ldflags", "ldflags")
def platform_cppflags(p): return platform_specific_values(p, "_cppflags", "cppflags")
def platform_ccasflags(p): return platform_specific_values(p, "_ccasflags", "ccasflags")
def platform_stripflags(p): return platform_specific_values(p, "_stripflags", "stripflags")
def platform_objcopyflags(p): return platform_specific_values(p, "_objcopyflags", "objcopyflags")

def module(platform):
    r = set_canonical_name_suffix(".module")

    r += gvar_add("noinst_PROGRAMS", "[+ name +].module")
    r += gvar_add("MODULE_FILES", "[+ name +].module$(EXEEXT)")

    r += var_set(cname() + "_SOURCES", platform_sources(platform) + " ## platform sources")
    r += var_set("nodist_" + cname() + "_SOURCES", platform_nodist_sources(platform) + " ## platform nodist sources")
    r += var_set(cname() + "_LDADD", platform_ldadd(platform))
    r += var_set(cname() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_MODULE) " + platform_cflags(platform))
    r += var_set(cname() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_MODULE) " + platform_ldflags(platform))
    r += var_set(cname() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_MODULE) " + platform_cppflags(platform))
    r += var_set(cname() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_MODULE) " + platform_ccasflags(platform))
    # r += var_set(cname() + "_DEPENDENCIES", platform_dependencies(platform) + " " + platform_ldadd(platform))

    r += gvar_add("EXTRA_DIST", extra_dist())
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + cname() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + cname() + "_SOURCES)")

    r += gvar_add("DEF_FILES", "def-[+ name +].lst")
    r += gvar_add("UND_FILES", "und-[+ name +].lst")
    r += gvar_add("MOD_FILES", "[+ name +].mod")
    r += gvar_add("platform_DATA", "[+ name +].mod")
    r += gvar_add("CLEANFILES", "def-[+ name +].lst und-[+ name +].lst mod-[+ name +].c mod-[+ name +].o [+ name +].mod")

    r += gvar_add("COMMAND_FILES", "command-[+ name +].lst")
    r += gvar_add("FS_FILES", "fs-[+ name +].lst")
    r += gvar_add("VIDEO_FILES", "video-[+ name +].lst")
    r += gvar_add("PARTMAP_FILES", "partmap-[+ name +].lst")
    r += gvar_add("HANDLER_FILES", "handler-[+ name +].lst")
    r += gvar_add("PARTTOOL_FILES", "parttool-[+ name +].lst")
    r += gvar_add("TERMINAL_FILES", "terminal-[+ name +].lst")
    r += gvar_add("CLEANFILES", "command-[+ name +].lst fs-[+ name +].lst")
    r += gvar_add("CLEANFILES", "handler-[+ name +].lst terminal-[+ name +].lst")
    r += gvar_add("CLEANFILES", "video-[+ name +].lst partmap-[+ name +].lst parttool-[+ name +].lst")

    r += gvar_add("CLEANFILES", "[+ name +].pp")
    r += """
[+ name +].pp: $(""" + cname() + """_SOURCES) $(nodist_""" + cname() + """_SOURCES)
	$(TARGET_CPP) -DGRUB_LST_GENERATOR $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(""" + cname() + """_CPPFLAGS) $(CPPFLAGS) $^ > $@ || (rm -f $@; exit 1)

def-[+ name +].lst: [+ name +].module$(EXEEXT)
	if test x$(USE_APPLE_CC_FIXES) = xyes; then \
	  $(NM) -g -P -p $< | grep -E '^[a-zA-Z0-9_]* [TDS]' | sed "s/^\\([^ ]*\\).*/\\1 [+ name +]/" >> $@; \
	else \
	  $(NM) -g --defined-only -P -p $< | sed "s/^\\([^ ]*\\).*/\\1 [+ name +]/" >> $@; \
	fi

und-[+ name +].lst: [+ name +].module$(EXEEXT)
	$(NM) -u -P -p $< | sed "s/^\\([^ ]*\\).*/\\1 [+ name +]/" >> $@

mod-[+ name +].c: [+ name +].module$(EXEEXT) moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh [+ name +] moddep.lst > $@ || (rm -f $@; exit 1)

mod-[+ name +].o: mod-[+ name +].c
	$(TARGET_CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(""" + cname() + """_CPPFLAGS) $(CPPFLAGS) $(""" + cname() + """_CFLAGS) $(CFLAGS) -c -o $@ $<

[+ name +].mod: [+ name +].module$(EXEEXT) mod-[+ name +].o
	if test x$(USE_APPLE_CC_FIXES) = xyes; then \
	  $(CCLD) $(""" + cname() + """_LDFLAGS) $(LDFLAGS) -o $@.bin $^; \
	  $(OBJCONV) -f$(TARGET_MODULE_FORMAT) -nr:_grub_mod_init:grub_mod_init -nr:_grub_mod_fini:grub_mod_fini -wd1106 -nu -nd $@.bin $@; \
	  rm -f $@.bin; \
	else \
	  $(CCLD) -o $@ $(""" + cname() + """_LDFLAGS) $(LDFLAGS) $^; \
	  if test ! -z '$(TARGET_OBJ2ELF)'; then $(TARGET_OBJ2ELF) $@ || (rm -f $@; exit 1); fi; \
	  $(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -K _grub_mod_init -K _grub_mod_fini -R .note -R .comment $@; \
	fi

command-[+ name +].lst: [+ name +].pp $(srcdir)/gencmdlist.sh
	cat $< | sh $(srcdir)/gencmdlist.sh [+ name +] > $@ || (rm -f $@; exit 1)

fs-[+ name +].lst: [+ name +].pp $(srcdir)/genfslist.sh
	cat $< | sh $(srcdir)/genfslist.sh [+ name +] > $@ || (rm -f $@; exit 1)

video-[+ name +].lst: [+ name +].pp $(srcdir)/genvideolist.sh
	cat $< | sh $(srcdir)/genvideolist.sh [+ name +] > $@ || (rm -f $@; exit 1)

partmap-[+ name +].lst: [+ name +].pp $(srcdir)/genpartmaplist.sh
	cat $< | sh $(srcdir)/genpartmaplist.sh [+ name +] > $@ || (rm -f $@; exit 1)

parttool-[+ name +].lst: [+ name +].pp $(srcdir)/genparttoollist.sh
	cat $< | sh $(srcdir)/genparttoollist.sh [+ name +] > $@ || (rm -f $@; exit 1)

handler-[+ name +].lst: [+ name +].pp $(srcdir)/genhandlerlist.sh
	cat $< | sh $(srcdir)/genhandlerlist.sh [+ name +] > $@ || (rm -f $@; exit 1)

terminal-[+ name +].lst: [+ name +].pp $(srcdir)/genterminallist.sh
	cat $< | sh $(srcdir)/genterminallist.sh [+ name +] > $@ || (rm -f $@; exit 1)
"""
    return r

def kernel(platform):
    r = set_canonical_name_suffix(".exec")
    r += gvar_add("noinst_PROGRAMS", "[+ name +].exec")
    r += var_set(cname() + "_SOURCES", platform_startup(platform))
    r += var_add(cname() + "_SOURCES", platform_sources(platform))
    r += var_set("nodist_" + cname() + "_SOURCES", platform_nodist_sources(platform) + " ## platform nodist sources")
    r += var_set(cname() + "_LDADD", platform_ldadd(platform))
    r += var_set(cname() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_KERNEL) " + platform_cflags(platform))
    r += var_set(cname() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_KERNEL) " + platform_ldflags(platform))
    r += var_set(cname() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_KERNEL) " + platform_cppflags(platform))
    r += var_set(cname() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_KERNEL) " + platform_ccasflags(platform))
    r += var_set(cname() + "_STRIPFLAGS", "$(AM_STRIPFLAGS) $(STRIPFLAGS_KERNEL) " + platform_stripflags(platform))
    # r += var_set(cname() + "_DEPENDENCIES", platform_dependencies(platform) + " " + platform_ldadd(platform))

    r += gvar_add("EXTRA_DIST", extra_dist())
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + cname() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + cname() + "_SOURCES)")

    r += gvar_add("platform_DATA", "[+ name +].img")
    r += gvar_add("CLEANFILES", "[+ name +].img")
    r += rule("[+ name +].img", "[+ name +].exec$(EXEEXT)",
              if_platform_tagged(platform, "nostrip", "cp $< $@",
                                 "$(STRIP) $(" + cname() + "_STRIPFLAGS) -o $@ $<"))
    return r

def image(platform):
    r = set_canonical_name_suffix(".image")
    r += gvar_add("noinst_PROGRAMS", "[+ name +].image")
    r += var_set(cname() + "_SOURCES", platform_sources(platform))
    r += var_set("nodist_" + cname() + "_SOURCES", platform_nodist_sources(platform) + "## platform nodist sources")
    r += var_set(cname() + "_LDADD", platform_ldadd(platform))
    r += var_set(cname() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_IMAGE) " + platform_cflags(platform))
    r += var_set(cname() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_IMAGE) " + platform_ldflags(platform))
    r += var_set(cname() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_IMAGE) " + platform_cppflags(platform))
    r += var_set(cname() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_IMAGE) " + platform_ccasflags(platform))
    r += var_set(cname() + "_OBJCOPYFLAGS", "$(OBJCOPYFLAGS_IMAGE) " + platform_objcopyflags(platform))
    # r += var_set(cname() + "_DEPENDENCIES", platform_dependencies(platform) + " " + platform_ldadd(platform))

    r += gvar_add("EXTRA_DIST", extra_dist())
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + cname() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + cname() + "_SOURCES)")

    r += gvar_add("platform_DATA", "[+ name +].img")
    r += gvar_add("CLEANFILES", "[+ name +].img")
    r += rule("[+ name +].img", "[+ name +].image$(EXEEXT)", """
if test x$(USE_APPLE_CC_FIXES) = xyes; then \
  $(MACHO2IMG) $< $@; \
else \
  $(OBJCOPY) $(""" + cname() + """_OBJCOPYFLAGS) --strip-unneeded -R .note -R .comment -R .note.gnu.build-id -R .reginfo -R .rel.dyn $< $@; \
fi
""")
    return r

def library(platform):
    r = set_canonical_name_suffix("")
    r += gvar_add("noinst_LIBRARIES", "[+ name +]")
    r += var_set(cname() + "_SOURCES", platform_sources(platform))
    r += var_set("nodist_" + cname() + "_SOURCES", platform_nodist_sources(platform))
    r += var_set(cname() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_LIBRARY) " + platform_cflags(platform))
    r += var_set(cname() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_LIBRARY) " + platform_cppflags(platform))
    r += var_set(cname() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_LIBRARY) " + platform_ccasflags(platform))
    # r += var_set(cname() + "_DEPENDENCIES", platform_dependencies(platform) + " " + platform_ldadd(platform))

    r += gvar_add("EXTRA_DIST", extra_dist())
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + cname() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + cname() + "_SOURCES)")

    return r

def installdir(default="bin"):
    return "[+ IF installdir +][+ installdir +][+ ELSE +]" + default + "[+ ENDIF +]"

def manpage():
    r  = "if COND_MAN_PAGES\n"
    r += gvar_add("man_MANS", "[+ name +].[+ mansection +]\n")
    r += rule("[+ name +].[+ mansection +]", "[+ name +]", """
chmod a+x [+ name +]
PATH=$(builddir):$$PATH $(HELP2MAN) --section=[+ mansection +] -i $(top_srcdir)/docs/man/[+ name +].h2m -o $@ [+ name +]
""")
    r += gvar_add("CLEANFILES", "[+ name +].[+ mansection +]")
    r += "endif\n"
    return r

def program(platform, test=False):
    r = set_canonical_name_suffix("")

    r += "[+ IF testcase defined +]"
    r += gvar_add("check_PROGRAMS", "[+ name +]")
    r += gvar_add("TESTS", "[+ name +]")
    r += "[+ ELSE +]"
    r += gvar_add(installdir() + "_PROGRAMS", "[+ name +]")
    r += "[+ IF mansection +]" + manpage() + "[+ ENDIF +]"
    r += "[+ ENDIF +]"

    r += var_set(cname() + "_SOURCES", platform_sources(platform))
    r += var_set("nodist_" + cname() + "_SOURCES", platform_nodist_sources(platform))
    r += var_set(cname() + "_LDADD", platform_ldadd(platform))
    r += var_set(cname() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_PROGRAM) " + platform_cflags(platform))
    r += var_set(cname() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_PROGRAM) " + platform_ldflags(platform))
    r += var_set(cname() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_PROGRAM) " + platform_cppflags(platform))
    r += var_set(cname() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_PROGRAM) " + platform_ccasflags(platform))
    # r += var_set(cname() + "_DEPENDENCIES", platform_dependencies(platform) + " " + platform_ldadd(platform))

    r += gvar_add("EXTRA_DIST", extra_dist())
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + cname() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + cname() + "_SOURCES)")
    return r

def data(platform):
    r  = gvar_add("EXTRA_DIST", platform_sources(platform))
    r += gvar_add("EXTRA_DIST", extra_dist())
    r += gvar_add(installdir() + "_DATA", platform_sources(platform))
    return r

def script(platform):
    r  = "[+ IF testcase defined +]"
    r += gvar_add("check_SCRIPTS", "[+ name +]")
    r += gvar_add ("TESTS", "[+ name +]")
    r += "[+ ELSE +]"
    r += gvar_add(installdir() + "_SCRIPTS", "[+ name +]")
    r += "[+ IF mansection +]" + manpage() + "[+ ENDIF +]"
    r += "[+ ENDIF +]"

    r += rule("[+ name +]", "$(top_builddir)/config.status " + platform_sources(platform), """
$(top_builddir)/config.status --file=-:""" + platform_sources(platform) + """ \
  | sed -e 's,@pkglib_DATA@,$(pkglib_DATA),g' > $@
chmod a+x [+ name +]
""")

    r += gvar_add("CLEANFILES", "[+ name +]")
    r += gvar_add("dist_noinst_DATA", platform_sources(platform))
    return r

def module_rules():
    return "[+ FOR module +]" + foreach_enabled_platform(
        lambda p: under_platform_specific_conditionals(p, module(p))) + "[+ ENDFOR +]"

def kernel_rules():
    return "[+ FOR kernel +]" + foreach_enabled_platform(
        lambda p: under_platform_specific_conditionals(p, kernel(p))) + "[+ ENDFOR +]"

def image_rules():
    return "[+ FOR image +]" + foreach_enabled_platform(
        lambda p: under_platform_specific_conditionals(p, image(p))) + "[+ ENDFOR +]"

def library_rules():
    return "[+ FOR library +]" + foreach_enabled_platform(
        lambda p: under_platform_specific_conditionals(p, library(p))) + "[+ ENDFOR +]"

def program_rules():
    return "[+ FOR program +]" + foreach_enabled_platform(
        lambda p: under_platform_specific_conditionals(p, program(p))) + "[+ ENDFOR +]"

def script_rules():
    return "[+ FOR script +]" + foreach_enabled_platform(
        lambda p: under_platform_specific_conditionals(p, script(p))) + "[+ ENDFOR +]"

def data_rules():
    return "[+ FOR data +]" + foreach_enabled_platform(
        lambda p: under_platform_specific_conditionals(p, data(p))) + "[+ ENDFOR +]"

print "[+ AutoGen5 template +]\n"
a = module_rules()
b = kernel_rules()
c = image_rules()
d = library_rules()
e = program_rules()
f = script_rules()
g = data_rules()
z = global_variable_initializers()

# print z # initializer for all vars
print a
print b
print c
print d
print e
print f
print g

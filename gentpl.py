#! /usr/bin/python

#
# This is the python script used to generate Makefile.tpl
#

GRUB_PLATFORMS = [ "emu", "i386_pc", "i386_efi", "i386_qemu", "i386_coreboot",
                   "i386_multiboot", "i386_ieee1275", "x86_64_efi",
                   "mips_yeeloong", "sparc64_ieee1275",
                   "powerpc_ieee1275" ]

GROUPS = {}
GROUPS["i386"]    = [ "i386_pc", "i386_efi", "i386_qemu", "i386_coreboot", "i386_multiboot", "i386_ieee1275" ]
GROUPS["x86_64"]  = [ "x86_64_efi" ]
GROUPS["mips"]    = [ "mips_yeeloong" ]
GROUPS["sparc64"] = [ "sparc64_ieee1275" ]
GROUPS["powerpc"] = [ "powerpc_ieee1275" ]
GROUPS["x86"]     = GROUPS["i386"] + GROUPS["x86_64"]
GROUPS["x86_efi"] = [ "i386_efi", "x86_64_efi" ]
GROUPS["nonemu"]  = GRUB_PLATFORMS[:]
GROUPS["nonemu"].remove("emu")

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

def set_canonical_name_suffix(suffix): return "[+ % name `export cname=$(echo -n %s" + suffix + " | sed -e 's/[^0-9A-Za-z@_]/_/g')` +]"
def cname(): return "[+ % name `echo $cname` +]"

def rule(target, source, cmd):
    if cmd[0] == "\n":
        return "\n" + target + ": " + source + cmd.replace("\n", "\n\t") + "\n"
    else:
        return "\n" + target + ": " + source + "\n\t" + cmd.replace("\n", "\n\t") + "\n"

def if_platform_tagged(platform, tag, closure, c2=None):
    r = ""
    r += "[+ IF " + tag + " defined +]"
    r += "[+ FOR " + tag + " +][+ CASE " + tag + " +]"
    for group in RMAP[platform]:
        r += "[+ = \"" + group + "\" +]" + closure()
    r += "[+ ESAC +][+ ENDFOR +]"

    if c2 == None:
        r += "[+ ENDIF +]"
        return r

    r += "[+ ELSE +]" + c2() + "[+ ENDIF +]"
    return r

def platform_values(platform, group_tag, default_tag):
    r = ""
    for group in RMAP[platform]:
        gtag = group + group_tag

        if group == RMAP[platform][0]:
            r += "[+ IF " + gtag + " +]"
        else:
            r += "[+ ELIF " + gtag + " +]"

        r += "[+ FOR " + gtag + " +][+ ." + gtag + " +] [+ ENDFOR +]"
    r += "[+ ELSE +][+ FOR " + default_tag + " +][+ ." + default_tag + " +] [+ ENDFOR +][+ ENDIF +]"
    return r

def under_conditional(x):
    return "[+ IF condition +]\nif [+ condition +]\n[+ ENDIF +]" + x + "[+ IF condition +]\nendif\n[+ ENDIF +]"

def each_platform(closure):
    r = "[+ IF - enable undefined +]"
    for platform in GRUB_PLATFORMS:
        r += "\nif COND_" + platform + "\n" + closure(platform) + "endif\n"
    r += "[+ ELSE +]"
    for platform in GRUB_PLATFORMS:
        x = "\nif COND_" + platform + "\n" + closure(platform) + "endif\n"
        r += if_platform_tagged(platform, "enable", lambda: x)
    r += "[+ ENDIF +]"
    return r;

def shared_sources(): return "[+ FOR shared +][+ .shared +] [+ ENDFOR +]"
def shared_nodist_sources(): return "[+ FOR nodist_shared +] [+ .nodist_shared +][+ ENDFOR +]"

def platform_sources(p): return platform_values(p, "", "source")
def platform_nodist_sources(p): return platform_values(p, "_nodist", "nodist")
def platform_extra_dist(p): return platform_values(p, "_extra_dist", "extra_dist")

def platform_ldadd(p): return platform_values(p, "_ldadd", "ldadd")
def platform_cflags(p): return platform_values(p, "_cflags", "cflags")
def platform_ldflags(p): return platform_values(p, "_ldflags", "ldflags")
def platform_cppflags(p): return platform_values(p, "_cppflags", "cppflags")
def platform_ccasflags(p): return platform_values(p, "_ccasflags", "ccasflags")
def platform_stripflags(p): return platform_values(p, "_stripflags", "stripflags")
def platform_objcopyflags(p): return platform_values(p, "_objcopyflags", "objcopyflags")

def module(platform):
    r = set_canonical_name_suffix(".module")

    r += gvar_add("noinst_PROGRAMS", "[+ name +].module")
    r += gvar_add("MODULE_FILES", "[+ name +].module$(EXEEXT)")

    r += var_set(cname() + "_SOURCES", platform_sources(platform) + " ## platform sources")
    r += var_add(cname() + "_SOURCES", shared_sources() + " ## shared sources")
    r += var_set("nodist_" + cname() + "_SOURCES", platform_nodist_sources(platform) + " ## platform nodist sources")
    r += var_add("nodist_" + cname() + "_SOURCES", shared_nodist_sources() + " ## shared nodist sources")
    r += var_set(cname() + "_LDADD", platform_ldadd(platform))
    r += var_set(cname() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_MODULE) " + platform_cflags(platform))
    r += var_set(cname() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_MODULE) " + platform_ldflags(platform))
    r += var_set(cname() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_MODULE) " + platform_cppflags(platform))
    r += var_set(cname() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_MODULE) " + platform_ccasflags(platform))

    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
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

mod-[+ name +].c: [+ name +].module$(EXEEXT) $(top_builddir)/moddep.lst $(top_srcdir)/genmodsrc.sh
	sh $(top_srcdir)/genmodsrc.sh [+ name +] $(top_builddir)/moddep.lst > $@ || (rm -f $@; exit 1)

mod-[+ name +].o: mod-[+ name +].c
	$(TARGET_CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS_MODULE) $(CPPFLAGS) $(CFLAGS_MODULE) $(CFLAGS) -c -o $@ $<

[+ name +].mod: [+ name +].module$(EXEEXT) mod-[+ name +].o
	if test x$(USE_APPLE_CC_FIXES) = xyes; then \
	  $(CCLD) $(LDFLAGS_MODULE) $(LDFLAGS) -o $@.bin $^; \
	  $(OBJCONV) -f$(TARGET_MODULE_FORMAT) -nr:_grub_mod_init:grub_mod_init -nr:_grub_mod_fini:grub_mod_fini -wd1106 -nu -nd $@.bin $@; \
	  rm -f $@.bin; \
	else \
	  $(CCLD) -o $@ $(LDFLAGS_MODULE) $(LDFLAGS) $^; \
	  if test ! -z '$(TARGET_OBJ2ELF)'; then $(TARGET_OBJ2ELF) $@ || (rm -f $@; exit 1); fi; \
	  $(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -K _grub_mod_init -K _grub_mod_fini -R .note -R .comment $@; \
	fi

command-[+ name +].lst: [+ name +].pp $(top_srcdir)/gencmdlist.sh
	cat $< | sh $(top_srcdir)/gencmdlist.sh [+ name +] > $@ || (rm -f $@; exit 1)

fs-[+ name +].lst: [+ name +].pp $(top_srcdir)/genfslist.sh
	cat $< | sh $(top_srcdir)/genfslist.sh [+ name +] > $@ || (rm -f $@; exit 1)

video-[+ name +].lst: [+ name +].pp $(top_srcdir)/genvideolist.sh
	cat $< | sh $(top_srcdir)/genvideolist.sh [+ name +] > $@ || (rm -f $@; exit 1)

partmap-[+ name +].lst: [+ name +].pp $(top_srcdir)/genpartmaplist.sh
	cat $< | sh $(top_srcdir)/genpartmaplist.sh [+ name +] > $@ || (rm -f $@; exit 1)

parttool-[+ name +].lst: [+ name +].pp $(top_srcdir)/genparttoollist.sh
	cat $< | sh $(top_srcdir)/genparttoollist.sh [+ name +] > $@ || (rm -f $@; exit 1)

handler-[+ name +].lst: [+ name +].pp $(top_srcdir)/genhandlerlist.sh
	cat $< | sh $(top_srcdir)/genhandlerlist.sh [+ name +] > $@ || (rm -f $@; exit 1)

terminal-[+ name +].lst: [+ name +].pp $(top_srcdir)/genterminallist.sh
	cat $< | sh $(top_srcdir)/genterminallist.sh [+ name +] > $@ || (rm -f $@; exit 1)
"""
    return r

def kernel(platform):
    r = set_canonical_name_suffix(".exec")
    r += gvar_add("noinst_PROGRAMS", "[+ name +].exec")
    r += var_set(cname() + "_SOURCES", platform_sources(platform))
    r += var_add(cname() + "_SOURCES", shared_sources())
    r += var_set("nodist_" + cname() + "_SOURCES", platform_nodist_sources(platform) + " ## platform nodist sources")
    r += var_add("nodist_" + cname() + "_SOURCES", shared_nodist_sources() + " ## shared nodist sources")
    r += var_set(cname() + "_LDADD", platform_ldadd(platform))
    r += var_set(cname() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_KERNEL) " + platform_cflags(platform))
    r += var_set(cname() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_KERNEL) " + platform_ldflags(platform))
    r += var_set(cname() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_KERNEL) " + platform_cppflags(platform))
    r += var_set(cname() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_KERNEL) " + platform_ccasflags(platform))
    r += var_set(cname() + "_STRIPFLAGS", "$(AM_STRIPFLAGS) $(STRIPFLAGS_KERNEL) " + platform_stripflags(platform))

    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + cname() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + cname() + "_SOURCES)")

    r += gvar_add("platform_DATA", "[+ name +].img")
    r += gvar_add("CLEANFILES", "[+ name +].img")
    r += rule("[+ name +].img", "[+ name +].exec$(EXEEXT)",
              if_platform_tagged(platform, "nostrip", lambda: "cp $< $@",
                                 lambda: "$(STRIP) $(" + cname() + "_STRIPFLAGS) -o $@ $<"))
    return r

def image(platform):
    r = set_canonical_name_suffix(".image")
    r += gvar_add("noinst_PROGRAMS", "[+ name +].image")
    r += var_set(cname() + "_SOURCES", platform_sources(platform))
    r += var_add(cname() + "_SOURCES", shared_sources())
    r += var_set("nodist_" + cname() + "_SOURCES", platform_nodist_sources(platform) + "## platform nodist sources")
    r += var_add("nodist_" + cname() + "_SOURCES", shared_nodist_sources() + "## shared nodist sources")
    r += var_set(cname() + "_LDADD", platform_ldadd(platform))
    r += var_set(cname() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_IMAGE) " + platform_cflags(platform))
    r += var_set(cname() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_IMAGE) " + platform_ldflags(platform))
    r += var_set(cname() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_IMAGE) " + platform_cppflags(platform))
    r += var_set(cname() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_IMAGE) " + platform_ccasflags(platform))

    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + cname() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + cname() + "_SOURCES)")

    r += gvar_add("platform_DATA", "[+ name +].img")
    r += gvar_add("CLEANFILES", "[+ name +].img")
    r += rule("[+ name +].img", "[+ name +].image$(EXEEXT)", """
if test x$(USE_APPLE_CC_FIXES) = xyes; then \
  $(MACHO2IMG) $< $@; \
else \
  $(OBJCOPY) """ + platform_objcopyflags(platform) + """ --strip-unneeded -R .note -R .comment -R .note.gnu.build-id -R .reginfo -R .rel.dyn $< $@; \
fi
""")
    return r

def library(platform):
    r = set_canonical_name_suffix("")
    r += gvar_add("noinst_LIBRARIES", "[+ name +]")
    r += var_set(cname() + "_SOURCES", platform_sources(platform))
    r += var_add(cname() + "_SOURCES", shared_sources())
    r += var_set("nodist_" + cname() + "_SOURCES", platform_nodist_sources(platform))
    r += var_add("nodist_" + cname() + "_SOURCES", shared_nodist_sources())
    r += var_set(cname() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_LIBRARY) " + platform_cflags(platform))
    r += var_set(cname() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_LIBRARY) " + platform_cppflags(platform))
    r += var_set(cname() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_LIBRARY) " + platform_ccasflags(platform))

    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + cname() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + cname() + "_SOURCES)")

    return r

def installdir(default="bin"):
    return "[+ IF installdir +][+ installdir +][+ ELSE +]" + default + "[+ ENDIF +]"

def manpage():
    r  = "if COND_MAN_PAGES\n"
    r += gvar_add("man_MANS", "[+ name +].[+ mansection +]\n")
    r += rule("[+ name +].[+ mansection +]", "", """
$(MAKE) $(AM_MAKEFLAGS) [+ name +]
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
    r += var_add(cname() + "_SOURCES", shared_sources())
    r += var_set("nodist_" + cname() + "_SOURCES", platform_nodist_sources(platform))
    r += var_add("nodist_" + cname() + "_SOURCES", shared_nodist_sources())
    r += var_set(cname() + "_LDADD", platform_ldadd(platform))
    r += var_set(cname() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_PROGRAM) " + platform_cflags(platform))
    r += var_set(cname() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_PROGRAM) " + platform_ldflags(platform))
    r += var_set(cname() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_PROGRAM) " + platform_cppflags(platform))
    r += var_set(cname() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_PROGRAM) " + platform_ccasflags(platform))

    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + cname() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + cname() + "_SOURCES)")
    return r

def data(platform):
    r  = gvar_add("EXTRA_DIST", platform_sources(platform))
    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
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
    r += gvar_add("EXTRA_DIST", platform_sources(platform))
    return r

def module_rules():
    return "[+ FOR module +]" + under_conditional(each_platform(lambda p: module(p))) + "[+ ENDFOR +]"

def kernel_rules():
    return "[+ FOR kernel +]" + under_conditional(each_platform(lambda p: kernel(p))) + "[+ ENDFOR +]"

def image_rules():
    return "[+ FOR image +]" + under_conditional(each_platform(lambda p: image(p))) + "[+ ENDFOR +]"

def library_rules():
    return "[+ FOR library +]" + under_conditional(each_platform(lambda p: library(p))) + "[+ ENDFOR +]"

def program_rules():
    return "[+ FOR program +]" + under_conditional(each_platform(lambda p: program(p))) + "[+ ENDFOR +]"

def script_rules():
    return "[+ FOR script +]" + under_conditional(each_platform(lambda p: script(p))) + "[+ ENDFOR +]"

def data_rules():
    return "[+ FOR data +]" + under_conditional(each_platform(lambda p: data(p))) + "[+ ENDFOR +]"

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

print """.PRECIOUS: modules.am
$(srcdir)/modules.am: $(srcdir)/modules.def $(top_srcdir)/Makefile.tpl
	autogen -T $(top_srcdir)/Makefile.tpl $(srcdir)/modules.def | sed -e '/^$$/{N;/^\\n$$/D;}' > $@.new || (rm -f $@.new; exit 1)
	mv $@.new $@

.PRECIOUS: $(top_srcdir)/Makefile.tpl
$(top_srcdir)/Makefile.tpl: $(top_srcdir)/gentpl.py
	python $(top_srcdir)/gentpl.py | sed -e '/^$$/{N;/^\\n$$/D;}' > $@.new || (rm -f $@.new; exit 1)
	mv $@.new $@
"""

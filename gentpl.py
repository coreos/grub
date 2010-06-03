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
GROUPS["common"]  = GRUB_PLATFORMS[:]
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

def if_tag(tag, closure):
    return "[+ IF " + tag + " +]" + closure() + "[+ ENDIF +]"

def if_tag_defined(tag, closure):
    return "[+ IF " + tag + " defined +]" + closure() + "[+ ENDIF +]"

def for_tag(tag, closure):
    return "[+ FOR ." + tag + " +]" + closure() + "[+ ENDFOR +]"

def collect_values(tag, prefix=""):
    return for_tag(tag, lambda: prefix + "[+ ." + tag + " +] ")

def each_group(platform, suffix, closure):
    r = None
    for group in RMAP[platform]:
        if r == None:
            r = "[+ IF ." + group + suffix + " +]"
        else:
            r += "[+ ELIF ." + group + suffix + " +]"
            
        r += closure(group)

    if r:
        r += "[+ ELSE +]"
        r += closure(None)
        r += "[+ ENDIF +]"
    else:
        r = closure(None)

    return r

def each_platform(closure):
    r = ""
    for platform in GRUB_PLATFORMS:
        for group in RMAP[platform]:
            if group == RMAP[platform][0]:
                r += "[+ IF ." + group + " defined +]"
            else:
                r += "[+ ELIF ." + group + " defined +]"

            r += "if COND_" + platform + "\n"
            r += closure(platform)
            r += "endif\n"
        r += "[+ ENDIF +]"
    return r

def canonical_name():   return "[+ % name `echo -n %s | sed -e 's/[^0-9A-Za-z@_]/_/g'` +]"
def canonical_module(): return canonical_name() + "_module"
def canonical_kernel(): return canonical_name() + "_img"
def canonical_image(): return canonical_name() + "_image"

def shared_sources(prefix=""):        return collect_values("shared", prefix)
def shared_nodist_sources(prefix=""): return collect_values("nodist_shared", prefix)

def default_sources(prefix=""):        return collect_values("source", prefix)
def default_nodist_sources(prefix=""): return collect_values("nodist", prefix)
def default_ldadd():        return collect_values("ldadd")
def default_cflags():       return collect_values("cflags")
def default_ldflags():      return collect_values("ldflags")
def default_cppflags():     return collect_values("cppflags")
def default_ccasflags():    return collect_values("ccasflags")
def default_extra_dist(): return collect_values("extra_dist")

def group_sources(group, prefix=""):        return collect_values(group, prefix) if group else default_sources(prefix)
def group_nodist_sources(group, prefix=""): return collect_values(group + "_nodist", prefix) if group else default_nodist_sources(prefix)

def platform_sources(platform, prefix=""):        return each_group(platform, "", lambda g: collect_values(g, prefix) if g else default_sources(prefix))
def platform_nodist_sources(platform, prefix=""): return each_group(platform, "_nodist", lambda g: collect_values(g + "_nodist", prefix) if g else default_nodist_sources(prefix))

def platform_ldadd(platform):        return each_group(platform, "_ldadd", lambda g: collect_values(g + "_ldadd") if g else default_ldadd())
def platform_cflags(platform):       return each_group(platform, "_cflags", lambda g: collect_values(g + "_cflags") if g else default_cflags())
def platform_ldflags(platform):      return each_group(platform, "_ldflags", lambda g: collect_values(g + "_ldflags") if g else default_ldflags())
def platform_cppflags(platform):     return each_group(platform, "_cppflags", lambda g: collect_values(g + "_cppflags") if g else default_cppflags())
def platform_ccasflags(platform):    return each_group(platform, "_ccasflags", lambda g: collect_values(g + "_ccasflags") if g else default_ccasflags())
def platform_extra_dist(platform): return each_group(platform, "_extra_dist", lambda g: collect_values(g + "_extra_dist") if g else default_extra_dist())
def platform_format(platform):       return each_group(platform, "_format", lambda g: collect_values(g + "_format") if g else "binary")

def module(platform):
    r  = gvar_add("noinst_PROGRAMS", "[+ name +].module")
    r += gvar_add("MODULE_FILES", "[+ name +].module")

    r += var_set(canonical_module() + "_SOURCES", platform_sources(platform) + "## platform sources")
    r += var_add(canonical_module() + "_SOURCES", shared_sources() + "## shared sources")
    r += var_set("nodist_" + canonical_module() + "_SOURCES", platform_nodist_sources(platform) + "## platform nodist sources")
    r += var_add("nodist_" + canonical_module() + "_SOURCES", shared_nodist_sources() + "## shared nodist sources")
    r += var_set(canonical_module() + "_LDADD", platform_ldadd(platform))
    r += var_set(canonical_module() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_MODULE) " + platform_cflags(platform))
    r += var_set(canonical_module() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_MODULE) " + platform_ldflags(platform))
    r += var_set(canonical_module() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_MODULE) " + platform_cppflags(platform))
    r += var_set(canonical_module() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_MODULE) " + platform_ccasflags(platform))

    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + canonical_module() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + canonical_module() + "_SOURCES)")

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
[+ name +].pp: $(""" + canonical_module() + """_SOURCES) $(nodist_""" + canonical_module() + """_SOURCES)
	$(TARGET_CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(""" + canonical_module() + """_CPPFLAGS) $(CPPFLAGS) $^ > $@ || (rm -f $@; exit 1)

def-[+ name +].lst: [+ name +].module
	if test x$(USE_APPLE_CC_FIXES) = xyes; then \
	  $(NM) -g -P -p $< | grep -E '^[a-zA-Z0-9_]* [TDS]' | sed "s/^\\([^ ]*\\).*/\\1 [+ name +]/" >> $@; \
	else \
	  $(NM) -g --defined-only -P -p $< | sed "s/^\\([^ ]*\\).*/\\1 [+ name +]/" >> $@; \
	fi

und-[+ name +].lst: [+ name +].module
	$(NM) -u -P -p $< | sed "s/^\\([^ ]*\\).*/\\1 [+ name +]/" >> $@

mod-[+ name +].c: [+ name +].module $(top_builddir)/moddep.lst $(top_srcdir)/genmodsrc.sh
	sh $(top_srcdir)/genmodsrc.sh [+ name +] $(top_builddir)/moddep.lst > $@ || (rm -f $@; exit 1)

mod-[+ name +].o: mod-[+ name +].c
	$(TARGET_CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS_MODULE) $(CPPFLAGS) $(CFLAGS_MODULE) $(CFLAGS) -c -o $@ $<

[+ name +].mod: [+ name +].module mod-[+ name +].o
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

def rule(target, source, cmd):
    if cmd[0] == "\n":
        return "\n" + target + ": " + source + cmd.replace("\n", "\n\t") + "\n"
    else:
        return "\n" + target + ": " + source + "\n\t" + cmd.replace("\n", "\n\t") + "\n"

def image_nostrip(platform):
    return if_tag_defined("image_nostrip." + platform, lambda: rule("[+ name +].img", "[+ name +].exec", "cp $< $@"))

def image_strip(platform):
    return if_tag_defined("image_strip." + platform, lambda: rule("[+ name +].img", "[+ name +].exec", "$(STRIP) -o $@ -R .rel.dyn -R .reginfo -R .note -R .comment $<"))

def image_strip_keep_kernel(platform):
    return if_tag_defined("image_strip_keep_kernel." + platform, lambda: rule("[+ name +].img", "[+ name +].exec", "$(STRIP) -o $@ --strip-unneeded -K start -R .note -R .comment $<"))

def image_strip_macho2img(platform):
    return if_tag_defined("image_strip_macho2img." + platform, lambda: rule("[+ name +].img", "[+ name +].exec", """
if test "x$(TARGET_APPLE_CC)" = x1; then \
  $(MACHO2IMG) --bss $< $@ || exit 1; \
else \
  $(STRIP) -o $@ -O binary --strip-unneeded -R .note -R .comment -R .note.gnu.build-id -R .reginfo -R .rel.dyn $< || exit 1; \
fi
"""))

def kernel(platform):
    r  = gvar_add("noinst_PROGRAMS", "[+ name +].img")
    r += var_set(canonical_kernel() + "_SOURCES", platform_sources(platform))
    r += var_add(canonical_kernel() + "_SOURCES", shared_sources())
    r += var_set("nodist_" + canonical_kernel() + "_SOURCES", platform_nodist_sources(platform) + "## platform nodist sources")
    r += var_add("nodist_" + canonical_kernel() + "_SOURCES", shared_nodist_sources() + "## shared nodist sources")
    r += var_set(canonical_kernel() + "_LDADD", platform_ldadd(platform))
    r += var_set(canonical_kernel() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_KERNEL) " + platform_cflags(platform))
    r += var_set(canonical_kernel() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_KERNEL) " + platform_ldflags(platform))
    r += var_set(canonical_kernel() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_KERNEL) " + platform_cppflags(platform))
    r += var_set(canonical_kernel() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_KERNEL) " + platform_ccasflags(platform))

    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + canonical_kernel() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + canonical_kernel() + "_SOURCES)")

    r += gvar_add("platform_DATA", "[+ name +].img")
    return r

def image(platform):
    r  = gvar_add("noinst_PROGRAMS", "[+ name +].image")
    r += var_set(canonical_image() + "_SOURCES", platform_sources(platform))
    r += var_add(canonical_image() + "_SOURCES", shared_sources())
    r += var_set("nodist_" + canonical_image() + "_SOURCES", platform_nodist_sources(platform) + "## platform nodist sources")
    r += var_add("nodist_" + canonical_image() + "_SOURCES", shared_nodist_sources() + "## shared nodist sources")
    r += var_set(canonical_image() + "_LDADD", platform_ldadd(platform))
    r += var_set(canonical_image() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_IMAGE) " + platform_cflags(platform))
    r += var_set(canonical_image() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_IMAGE) " + platform_ldflags(platform))
    r += var_set(canonical_image() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_IMAGE) " + platform_cppflags(platform))
    r += var_set(canonical_image() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_IMAGE) " + platform_ccasflags(platform))

    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + canonical_image() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + canonical_image() + "_SOURCES)")

    r += gvar_add("platform_DATA", "[+ name +].img")
    r += gvar_add("CLEANFILES", "[+ name +].img")
    r += rule("[+ name +].img", "[+ name +].image", """
if test x$(USE_APPLE_CC_FIXES) = xyes; then \
  $(MACHO2IMG) $< $@; \
else \
  $(OBJCOPY) -O """ + platform_format(platform) + """ --strip-unneeded -R .note -R .comment -R .note.gnu.build-id -R .reginfo -R .rel.dyn $< $@; \
fi
""")
    return r

def library(platform):
    r  = gvar_add("noinst_LIBRARIES", "[+ name +]")
    r += var_set(canonical_name() + "_SOURCES", platform_sources(platform))
    r += var_add(canonical_name() + "_SOURCES", shared_sources())
    r += var_set("nodist_" + canonical_name() + "_SOURCES", platform_nodist_sources(platform))
    r += var_add("nodist_" + canonical_name() + "_SOURCES", shared_nodist_sources())
    r += var_set(canonical_name() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_LIBRARY) " + platform_cflags(platform))
    r += var_set(canonical_name() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_LIBRARY) " + platform_cppflags(platform))
    r += var_set(canonical_name() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_LIBRARY) " + platform_ccasflags(platform))

    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + canonical_name() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + canonical_name() + "_SOURCES)")

    return r

def installdir(default="bin"):
    return "[+ IF installdir +][+ installdir +][+ ELSE +]" + default + "[+ ENDIF +]"

def manpage():
    r  = "if COND_MAN_PAGES\n"
    r += gvar_add("man_MANS", "[+ name +].[+ mansection +]\n")
    r += rule("[+ name +].[+ mansection +]", "", """
$(MAKE) $(AM_MAKEFLAGS) [+ name +]
chmod a+x [+ name +]
$(HELP2MAN) --section=[+ mansection +] -o $@ ./[+ name +]
""")
    r += gvar_add("CLEANFILES", "[+ name +].[+ mansection +]")
    r += "endif\n"
    return r

def program(platform, test=False):
    if test:
        r = gvar_add("check_PROGRAMS", "[+ name +]")
    else:
        r  = gvar_add(installdir() + "_PROGRAMS", "[+ name +]")

    r += var_set(canonical_name() + "_SOURCES", platform_sources(platform))
    r += var_add(canonical_name() + "_SOURCES", shared_sources())
    r += var_set("nodist_" + canonical_name() + "_SOURCES", platform_nodist_sources(platform))
    r += var_add("nodist_" + canonical_name() + "_SOURCES", shared_nodist_sources())
    r += var_set(canonical_name() + "_LDADD", platform_ldadd(platform))
    r += var_set(canonical_name() + "_CFLAGS", "$(AM_CFLAGS) $(CFLAGS_PROGRAM) " + platform_cflags(platform))
    r += var_set(canonical_name() + "_LDFLAGS", "$(AM_LDFLAGS) $(LDFLAGS_PROGRAM) " + platform_ldflags(platform))
    r += var_set(canonical_name() + "_CPPFLAGS", "$(AM_CPPFLAGS) $(CPPFLAGS_PROGRAM) " + platform_cppflags(platform))
    r += var_set(canonical_name() + "_CCASFLAGS", "$(AM_CCASFLAGS) $(CCASFLAGS_PROGRAM) " + platform_ccasflags(platform))

    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
    r += gvar_add("BUILT_SOURCES", "$(nodist_" + canonical_name() + "_SOURCES)")
    r += gvar_add("CLEANFILES", "$(nodist_" + canonical_name() + "_SOURCES)")

    if test:
        r += if_tag_defined("enable", lambda: gvar_add("TESTS", "[+ name +]"))
    else:
        r += if_tag("mansection", lambda: manpage())

    return r

def test_program(platform):
    return program(platform, True)

def data(platform):
    r  = gvar_add("EXTRA_DIST", platform_sources(platform))
    r += gvar_add("EXTRA_DIST", platform_extra_dist(platform))
    r += gvar_add(installdir() + "_DATA", platform_sources(platform))
    return r

def script(platform, test=False):
    if test:
        r = gvar_add("check_SCRIPTS", "[+ name +]")
    else:
        r  = gvar_add(installdir() + "_SCRIPTS", "[+ name +]")

    r += rule("[+ name +]", "$(top_builddir)/config.status " + platform_sources(platform), """
$(top_builddir)/config.status --file=-:""" + platform_sources(platform) + """ \
  | sed -e 's,@pkglib_DATA@,$(pkglib_DATA),g' > $@
chmod a+x [+ name +]
""")

    r += gvar_add("CLEANFILES", "[+ name +]")
    r += gvar_add("EXTRA_DIST", platform_sources(platform))

    if test:
        r += if_tag_defined("enable", lambda: gvar_add("TESTS", "[+ name +]"))
    else:
        r += if_tag("mansection", lambda: manpage())

    return r

def test_script(platform):
    return script(platform, True)

def with_enable_condition(x):
    return "[+ IF enable +]if [+ enable +]\n" + x + "endif\n[+ ELSE +]" + x + "[+ ENDIF +]"

def module_rules():
    return for_tag("module", lambda: with_enable_condition(each_platform(lambda p: module(p))))

def kernel_rules():
    return for_tag("kernel", lambda: with_enable_condition(each_platform(lambda p: kernel(p))))

def image_rules():
    return for_tag("image", lambda: with_enable_condition(each_platform(lambda p: image(p))))

def library_rules():
    return for_tag("library", lambda: with_enable_condition(each_platform(lambda p: library(p))))

def program_rules():
    return for_tag("program", lambda: with_enable_condition(each_platform(lambda p: program(p))))

def script_rules():
    return for_tag("script", lambda: with_enable_condition(each_platform(lambda p: script(p))))

def data_rules():
    return for_tag("data", lambda: with_enable_condition(each_platform(lambda p: data(p))))

def test_program_rules():
    return for_tag("test_program", lambda: with_enable_condition(each_platform(lambda p: test_program(p))))

def test_script_rules():
    return for_tag("test_script", lambda: with_enable_condition(each_platform(lambda p: test_script(p))))

print "[+ AutoGen5 template +]\n"
a = module_rules()
b = kernel_rules()
c = image_rules()
d = library_rules()
e = program_rules()
f = script_rules()
g = data_rules()
h = test_program_rules()
i = test_script_rules()
z = global_variable_initializers()

print z # initializer for all vars
print a
print b
print c
print d
print e
print f
print g
print h
print i

print """.PRECIOUS: modules.am
$(srcdir)/modules.am: $(srcdir)/modules.def $(top_srcdir)/Makefile.tpl
	autogen -T $(top_srcdir)/Makefile.tpl $(srcdir)/modules.def | sed -e '/^$$/{N;/^\\n$$/D;}' > $@.new || (rm -f $@.new; exit 1)
	mv $@.new $@

.PRECIOUS: $(top_srcdir)/Makefile.tpl
$(top_srcdir)/Makefile.tpl: $(top_srcdir)/gentpl.py
	python $(top_srcdir)/gentpl.py | sed -e '/^$$/{N;/^\\n$$/D;}' > $@.new || (rm -f $@.new; exit 1)
	mv $@.new $@
"""

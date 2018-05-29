print" "
print"Minicomputer-------------- "
print"-                     1/2:configuring"

if ARGUMENTS.get('64bit', 0):
	env = Environment(CCFLAGS = '-m64')
	# guienv = Environment(tools = [xgettext], CPPFLAGS = '-m64')
	guienv = Environment(CPPFLAGS = '-m64')
else:
	env = Environment(CCFLAGS = '')
	# guienv = Environment(tools = [xgettext], CPPFLAGS = '')
	guienv = Environment(CPPFLAGS = '')

if ARGUMENTS.get('k8', 0):
	env.Append(CCFLAGS = ['-march=k8','-mtune=k8','-m3dnow'])
	guienv.Append(CPPFLAGS = ['-march=k8','-mtune=k8'])
if ARGUMENTS.get('k7', 0):
	env.Append(CCFLAGS = ['-march=athlon-xp','-mtune=athlon-xp'])
	guienv.Append(CPPFLAGS = ['-march=athlon-xp','-mtune=athlon-xp'])
if ARGUMENTS.get('pg', 0):
	env.Append(CCFLAGS = ['-pg'])
	env.Append(LINKFLAGS = ['-pg'])

# env.Append(CCFLAGS = '  -O3 -mfpmath=sse -msse -msse2  -fverbose-asm  -ffast-math -funit-at-a-time -fpeel-loops -ftracer -funswitch-loops -Wall -fmessage-length=0')

env.Append(CCFLAGS = ['-g','-O2','-msse','-fwhole-program','-ftree-vectorize','-ffast-math', '-funit-at-a-time', '-fpeel-loops', '-ftracer','-funswitch-loops','-fprefetch-loop-arrays','-mfpmath=sse'])

if ARGUMENTS.get('native', 0):
	env.Append(CCFLAGS = ['-march=native','-mtune=native'])
	guienv.Append(CPPFLAGS = ['-march=native','-mtune=native'])
if ARGUMENTS.get('core2', 0):
	env.Append(CCFLAGS = ['-march=core2','-mtune=core2'])
	guienv.Append(CPPFLAGS = ['-march=core2','-mtune=core2'])
if ARGUMENTS.get('pentium-m', 0):
	env = Environment(CCFLAGS = [''])
	#env.Append(CCFLAGS = ['-march=pentium-m','-mtune=pentium-m'])
	#guienv.Append(CPPFLAGS = ['-march=pentium-m','-mtune=pentium-m'])
conf = Configure(env)

if not conf.CheckCXX():
    print('!! Your compiler and/or environment is not correctly configured.')
    Exit(1)
if not conf.CheckFunc('printf'):
    print('!! Your compiler and/or environment is not correctly configured.')
    Exit(1)
if not conf.CheckLibWithHeader('jack', 'jack/jack.h','c'):
	print 'Did not find jack, exiting!'
	Exit(1)
if not conf.CheckLibWithHeader('lo', 'lo/lo.h','c'):
	print 'Did not find liblo for OSC, exiting!'
	Exit(1)
if not conf.CheckLibWithHeader('asound', 'alsa/asoundlib.h','c'):
	print 'Did not find alsa, exiting!'
	Exit(1)
if not conf.CheckLibWithHeader('pthread', 'pthread.h','c'):
	print 'Did not find pthread library, exiting!'
	Exit(1)

env.Append(LIBS=['m'])
# env.Append(LINKFLAGS = ['-pg'])
env = conf.Finish()

print"-                    checking dependencies for the editor:"

guiconf = Configure(guienv)
if not guiconf.CheckLibWithHeader('lo', 'lo/lo.h','c'):
	print 'Did not find liblo for OSC, exiting!'
	Exit(1)
if not guiconf.CheckLibWithHeader('fltk', 'FL/Fl.H','c++'):
	print 'Did not find FLTK for the gui, exiting!'
	Exit(1)
if not guiconf.CheckLibWithHeader('asound', 'alsa/asoundlib.h','c'):
	print 'Did not find alsa, exiting!'
	Exit(1)
if not guiconf.CheckLibWithHeader('pthread', 'pthread.h','c'):
	print 'Did not find pthread library, exiting!'
	Exit(1)
guienv = guiconf.Finish()
guienv.Append(CPPFLAGS = ['-O3','-Wall','-fmessage-length=0'])

# see https://www.programcreek.com/python/example/94711/SCons.Action.Action
guienv.SetDefault(gettext_package_bugs_address="marc.perilleux@laposte.net")
guienv.SetDefault(gettext_package_name="")
guienv.SetDefault(gettext_package_version="")

guienv['BUILDERS']['gettextMoFile']=env.Builder(
		action=Action("msgfmt -o $TARGET $SOURCE", "Compiling translation from $SOURCE"),
		suffix=".mo",
		src_suffix=".po"
	)

XGETTEXT_COMMON_ARGS="--keyword=_ --language=C --add-comments --sort-output"

guienv['BUILDERS']['gettextPotFile']=env.Builder(
	action=Action("xgettext " + XGETTEXT_COMMON_ARGS + " -o $TARGET $SOURCE", "Generating pot file $TARGET"),
	suffix=".pot")

# Not tested
guienv['BUILDERS']['gettextMergePotFile']=env.Builder(
	action=Action("xgettext " + "--omit-header --no-location " + XGETTEXT_COMMON_ARGS + " -o $TARGET $SOURCE",
		"Generating pot file $TARGET"),
	suffix=".pot")

# Not working
guienv['BUILDERS']['msgmergePoFile']=env.Builder(
	action=Action("msgmerge --update $TARGET $SOURCE", "Merging po file $TARGET"),
	suffix=".po",
	src_suffix=".pot"
	)


print"-                     2/2:compiling"
print"-                     building the engine:"

env.Program('minicomputerCPU','cpu/main.c');

print""
print"-                     building the editor:"

guienv.Program('minicomputer',['editor/main.cpp','editor/Memory.cpp','editor/syntheditor.cxx','editor/MiniKnob.cxx','editor/MiniPositioner.cxx','editor/MiniTable.cxx']);

# guienv.gettextPotFile('editor/po/minicomputer.pot',['editor/syntheditor.cxx']);
# guienv.msgmergePoFile('editor/po/fr/minicomputer.po',['editor/po/minicomputer.pot']);
guienv.gettextMoFile('editor/po/fr/minicomputer.mo',['editor/po/fr/minicomputer.po']);

env.Alias(target="install", source=env.Install(dir="/usr/local/bin", source="minicomputer"));
env.Alias(target="install", source=env.Install(dir="/usr/local/bin", source="minicomputerCPU"));
env.Alias(target="install", source=env.Install(dir="/usr/share/locale/fr/LC_MESSAGES", source="editor/po/fr/minicomputer.mo"));

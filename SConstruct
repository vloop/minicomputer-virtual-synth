# sample use: scons native=1

import os.path

print" "
print"Minicomputer-------------- "
print"-                     1/2:configuring"

if ARGUMENTS.get('64bit', 0):
	# env = Environment(CCFLAGS = '-m64')
	env = Environment(CCFLAGS = '-m64', CONFIGURELOG = '#/../config_minicomputer.log')
	# guienv = Environment(tools = [xgettext], CPPFLAGS = '-m64')
	guienv = Environment(CPPFLAGS = '-m64', CONFIGURELOG = '#/../config_minicomputer.log')
else:
	# env = Environment(CCFLAGS = '')
	env = Environment(CCFLAGS = '', CONFIGURELOG = '#/../config_minicomputer.log')
	# guienv = Environment(tools = [xgettext], CPPFLAGS = '')
	guienv = Environment(CPPFLAGS = '', CONFIGURELOG = '#/../config_minicomputer.log')

# build options
# deprecated
opts = Options('scache.conf')
opts.AddOptions(
    PathOption('PREFIX', 'install prefix', '/usr'), # Was /usr/local
    PathOption('DESTDIR', 'intermediate install prefix', '', PathOption.PathAccept),
)
opts.Update(env)
opts.Save('scache.conf', env)
Help(opts.GenerateHelpText(env))
# should be something like
# opts = Variables()
# opts.Add('xxxx',0)
# env = Environment(variables=opts, ...)
#for key in opts.keys():
#    print key

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
# if not conf.CheckLibWithHeader('jack', 'jack/jack.h','c'):
#	print 'Did not find jack, exiting!'
#	Exit(1)
if not conf.CheckLib('libjack'):
	print 'Did not find jack library, exiting!'
	Exit(1)
if not conf.CheckHeader('jack/jack.h'):
	if not conf.CheckHeader('jack/jack.h'):
		print 'Did not find jack development files, exiting!'
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
# use the option below for profiling
# env.Append(LINKFLAGS = ['-pg'])

# install paths
prefix_bin = os.path.join(env['PREFIX'], 'bin')
prefix_share = os.path.join(env['PREFIX'], 'share')

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

XGETTEXT_COMMON_ARGS="--keyword=_ --language=C --add-comments --sort-output -o $TARGET $SOURCE"

guienv['BUILDERS']['gettextPotFile']=env.Builder(
	action=Action("xgettext --from-code=UTF-8 " + XGETTEXT_COMMON_ARGS, "Generating pot file $TARGET from $SOURCE"),
	suffix=".pot")

# Not tested
guienv['BUILDERS']['gettextMergePotFile']=env.Builder(
	action=Action("xgettext " + "--omit-header --no-location " + XGETTEXT_COMMON_ARGS,
		"Generating pot file $TARGET"),
	suffix=".pot")

# Not working (removes the .po before attempting merge because it's a target?)
# maybe could use Precious()
# msgmerge --update ./src/editor/po/fr/minicomputer.po ./src/editor/po/minicomputer.pot
guienv['BUILDERS']['msgmergePoFile']=env.Builder(
	action=Action("msgmerge --update $TARGET $SOURCE", "Merging po file $TARGET"),
	suffix=".po",
	src_suffix=".pot"
	)


print"-                     2/2:compiling"
print"-                     building the engine:"

env.Program('minicomputerCPU','src/cpu/minicomputerCPU.c');

print""
print"-                     building the editor:"

sources = ['src/editor/minicomputer.cpp',
  'src/editor/strnutil.cpp',
  'src/editor/Memory.cpp',
  'src/editor/syntheditor.cxx',
  'src/editor/MiniKnob.cxx',
  'src/editor/MiniButton.cxx',
  'src/editor/MiniPositioner.cxx',
  'src/editor/MiniTable.cxx',
  'src/editor/minichoice.cxx']
guienv.Program('minicomputer', sources);

guienv.gettextPotFile('src/editor/po/minicomputer.pot',['src/editor/syntheditor.cxx']); # OK
#guienv.gettextPotFile('src/editor/po/minicomputer.pot',['src/editor/*.cxx']); # Untested
#guienv.gettextPotFile('src/editor/po/minicomputer.pot',['src/editor/syntheditor.cxx', 'src/editor/minichoice.cxx']); # minichoice ignored??
# guienv.gettextPotFile('src/editor/po/minicomputer.pot', sources); # KO

# guienv.msgmergePoFile('src/editor/po/fr/minicomputer.po',['src/editor/po/minicomputer.pot']);
guienv.gettextMoFile('src/editor/po/fr/minicomputer.mo',['src/editor/po/fr/minicomputer.po']);

env.Alias('install', [
    env.Install(env['DESTDIR'] + prefix_bin, 'minicomputer'),
    env.Install(env['DESTDIR'] + prefix_bin, 'minicomputerCPU'),
    # should it be guienv below?
    env.Install(env['DESTDIR'] + os.path.join(prefix_share, 'locale/fr/LC_MESSAGES'), 'src/editor/po/fr/minicomputer.mo'),
    env.Install(env['DESTDIR'] + os.path.join(prefix_share, 'applications'), 'minicomputer.desktop'),
    env.Install(env['DESTDIR'] + os.path.join(prefix_share, 'pixmaps'), 'minicomputer.xpm'),
    env.Install(env['DESTDIR'] + os.path.join(prefix_share, 'minicomputer/factory_presets'), 'factory_presets/initsinglesound.txt'),
    env.Install(env['DESTDIR'] + os.path.join(prefix_share, 'minicomputer/factory_presets'), 'factory_presets/minicomputerMemory.txt'),
    env.Install(env['DESTDIR'] + os.path.join(prefix_share, 'minicomputer/factory_presets'), 'factory_presets/minicomputerMulti.txt')
])

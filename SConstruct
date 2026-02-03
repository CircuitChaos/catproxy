env = Environment()
env['CCFLAGS']	= '-Wall -Wextra -std=c++17 -O2 -pipe -g'
env['CPPPATH']	= 'src'
env['LIBS'] = ['X11']

env.VariantDir('build', 'src', duplicate = 0)
catproxy = env.Program('build/catproxy', Glob('build/*.cpp'))

env.Install('/usr/local/bin', catproxy)
env.Alias('install', '/usr/local/bin')

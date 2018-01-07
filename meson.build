project('ZLL Gateway' , 'c')

# Sources
src = ['src/main.c',
        'src/aps.c',
        'src/conf.c',
        'src/keys.c',
        'src/mt_af.c',
        'src/mt_sys.c',
        'src/mt_zdo.c',
        'src/mt_util.c',
        'src/zll.c',
        'src/utils/sm.c']

# Includes
znpinc = include_directories('/usr/local/include/libznp')
localinc = include_directories('include', 'include/utils')
incdir = [znpinc, localinc]

# Libraries
cc = meson.get_compiler('c')
znpdep = cc.find_library('znp', dirs: ['/usr/local/lib',
                                        '/usr/lib',
                                        '/usr/local/lib/x86_64-linux-gnu/'])
uvdep = cc.find_library('uv', dirs: ['/usr/local/lib', '/usr/lib'])
ptdep = dependency('threads')
iniparserdep = cc.find_library('iniparser')
dep = [znpdep, uvdep, ptdep, iniparserdep]

# Build options
cflags=['-Wall', '-Wextra', '-Werror']

executable('zll_gateway',
    sources: src,
    c_args: cflags,
    include_directories: incdir,
    dependencies: dep)

project('ZLL Gateway' , 'c')

# Sources
src = ['src/main.c',
        'src/app.c',
        'src/mt_sys.c']

# Includes
znpinc = include_directories('/usr/local/include/libznp')
localinc = include_directories('include')
incdir = [znpinc, localinc]

# Libraries
cc = meson.get_compiler('c')
znpdep = cc.find_library('znp', dirs: ['/usr/local/lib', '/usr/lib'])
uvdep = cc.find_library('uv', dirs: ['/usr/local/lib', '/usr/lib'])
ptdep = dependency('threads')
dep = [znpdep, uvdep, ptdep]

# Build options
cflags=['-Wall', '-Wextra', '-Werror']

executable('zll_gateway',
    sources: src,
    c_args: cflags,
    include_directories: incdir,
    dependencies: dep)

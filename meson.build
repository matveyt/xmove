project('xmove', 'c', default_options : ['buildtype=minsize', 'c_std=c99',
  'warning_level=3', 'werror=true'])


sources = ['xmove.c']
c_args = ['-fno-ident']
link_args = ['-s', '-Wl,--gc-sections']


if not get_option('ansi')
  c_args += ['-D_UNICODE', '-DUNICODE']
  link_args += '-municode'
endif


if get_option('nostdlib')
  sources += 'nocrt0c.c'
  link_args += ['-nostdlib', '-lkernel32', '-luser32', '-lshell32']
endif


executable('xmove', sources : sources, c_args : c_args, link_args : link_args)

fclist = executable('fc-list', ['fc-list.c', fcstdint_h, alias_headers, ft_alias_headers],
  include_directories: [incbase, incsrc],
  dependencies: [libintl_dep],
  link_with: [libfontconfig],
  c_args: c_args,
  install: true,
)

tools_man_pages += ['fc-list']

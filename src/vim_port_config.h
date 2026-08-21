// vim_port_config.h - Configuration for porting vim to KuzuOS5
// This disables all vim features we can't support and keeps only essentials

#ifndef VIM_PORT_CONFIG_H
#define VIM_PORT_CONFIG_H

// Disable GUI - we only support terminal
#define FEAT_GUI_NONE

// Disable features we won't implement
#undef FEAT_CLIPBOARD
#undef FEAT_MOUSE
#undef FEAT_SOUND
#undef FEAT_EVAL
#undef FEAT_LUA
#undef FEAT_PERL
#undef FEAT_PYTHON
#undef FEAT_RUBY
#undef FEAT_TCL
#undef FEAT_ARABIC
#undef FEAT_RIGHTLEFT
#undef FEAT_CINDENT
#undef FEAT_CSCOPE
#undef FEAT_NETBEANS_INTG

// Keep essential features only
#define FEAT_SEARCH_EXT
#define FEAT_CMDL_COMPL
#define FEAT_VISUAL
#define FEAT_DIFF
#define FEAT_SYNTAX
#define FEAT_MODELINE

// Memory configuration
#define LALLOC_CLEAR

// Minimal terminal features
#define FEAT_TERMRESPONSE

// Terminal type
#define TERM_ANSI

#endif /* VIM_PORT_CONFIG_H */

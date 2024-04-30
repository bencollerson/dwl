#include <X11/XF86keysym.h>
#define TERMINAL "x-terminal-emulator"
#define BROWSER "qutebrowser"

/* Taken from https://github.com/djpohly/dwl/issues/466 */
#define COLOR(hex)    { ((hex >> 24) & 0xFF) / 255.0f, \
                        ((hex >> 16) & 0xFF) / 255.0f, \
                        ((hex >> 8) & 0xFF) / 255.0f, \
                        (hex & 0xFF) / 255.0f }
/* appearance */
static const int sloppyfocus               = 1;  /* focus follows mouse */
static const int bypass_surface_visibility = 0;  /* 1 means idle inhibitors will disable idle tracking even if it's surface isn't visible  */
static const unsigned int borderpx         = 1;  /* border pixel of windows */
static const int lockfullscreen            = 1;  /* 1 will force focus on the fullscreen window */
static const int smartborders              = 1;

static const Placement floatplacement      = {0.75f, 0.75f};  /* horizontal, vertical positions */
static const Placement warpplacement       = {0.47f, 0.5f};   /* horizontal, vertical positions */

static const float backgrey[]              = COLOR(0x222222ff);
static const float focusyellow[]           = COLOR(0xfd971fff);
static const float black[]                 = COLOR(0x000000ff);
static const float urgentred[]             = COLOR(0xf92672ff);

static const float *rootcolor              = black;
static const float *bordercolor            = backgrey;
static const float *focuscolor             = focusyellow;
static const float *urgentcolor            = urgentred;
/* To conform the xdg-protocol, set the alpha to zero to restore the old behavior */
static const float *fullscreen_bg          = black;

/* tagging - tagcount should be no greater than 31 */
#define TAGCOUNT (26)

/* logging */
static int log_level = WLR_ERROR;

static const Rule rules[] = {
	/* app_id  title  monitor  tags  isfloating  hfact   wfact   titleurgent  scratchkey*/
	{ "org.keepassxc.KeePassXC",
		   NULL,  -1,      0,    1,          0.45f,  0.4f,   0,           'k' },
	{ "foot",  NULL,  -1,      0,    0,          0,      0,      0,           0  },
	{ "footclient",
		   NULL,  -1,      0,    0,          0,      0,      0,           0  },
	{ "udiskie",
		   NULL,  -1,      0,    1,          0.4f,   0.4f,   0,           0 },
	{ "net.sourceforge.xaos.xaos",
		   NULL,  -1,      0,    0,          0,      0,      0,           0 },
	{ "org.qutebrowser.qutebrowser",
		   NULL,  -1,      0,    0,          0,      0,      1,           0 },
	{ "brave-browser",
		   NULL,  -1,      0,    0,          0,      0,      1,           0 },
};

/* layout(s) */
static const Layout layouts[] = {
	/* symbol    arrange function */
	{ "||",      col },
	{ "|-",      tile },
	{ "[]",      NULL },    /* no layout function means floating behavior */
	{ "mo",      monocle },
	{ "##",      gaplessgrid },
};

enum layout_types {
	LAYOUT_COLUMN,
	LAYOUT_TILE,
	LAYOUT_FLOAT,
	LAYOUT_MONOCLE,
	LAYOUT_GAPLESSGRID,
};

/* monitors */
static const MonitorRule monrules[] = {
	/* name       mfact  scale layout       rotate/reflect */
	/* example of a HiDPI laptop monitor:
	{ "eDP-1",    0.5,  2,    &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL, -1, -1 },
	*/
	/* defaults */
	{ NULL,       0.55f, 1,    &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL, -1, -1 },
};

/* keyboard */
static const struct xkb_rule_names xkb_rules = {
	/* can specify fields: rules, model, layout, variant, options */
	/* example:
	.options = "ctrl:nocaps",
	*/
	.options = NULL,
};

static const int repeat_rate = 25;
static const int repeat_delay = 600;

/* Trackpad */
static const int tap_to_click = 1;
static const int tap_and_drag = 1;
static const int drag_lock = 1;
static const int natural_scrolling = 1;
static const int disable_while_typing = 1;
static const int left_handed = 0;
static const int middle_button_emulation = 0;
/* You can choose between:
LIBINPUT_CONFIG_SCROLL_NO_SCROLL
LIBINPUT_CONFIG_SCROLL_2FG
LIBINPUT_CONFIG_SCROLL_EDGE
LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN
*/
static const enum libinput_config_scroll_method scroll_method = LIBINPUT_CONFIG_SCROLL_2FG;

/* You can choose between:
LIBINPUT_CONFIG_CLICK_METHOD_NONE
LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS
LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER
*/
static const enum libinput_config_click_method click_method = LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;

/* You can choose between:
LIBINPUT_CONFIG_SEND_EVENTS_ENABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE
*/
static const uint32_t send_events_mode = LIBINPUT_CONFIG_SEND_EVENTS_ENABLED;

/* You can choose between:
LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT
LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE */
static const enum libinput_config_accel_profile accel_profile = LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
static const double accel_speed = 1.0;
static const int cursor_timeout = 5;
/* You can choose between:
LIBINPUT_CONFIG_TAP_MAP_LRM -- 1/2/3 finger tap maps to left/right/middle
LIBINPUT_CONFIG_TAP_MAP_LMR -- 1/2/3 finger tap maps to left/middle/right
*/
static const enum libinput_config_tap_button_map button_map = LIBINPUT_CONFIG_TAP_MAP_LMR;

/* If you want to use the windows key for MODKEY, use WLR_MODIFIER_LOGO */
#define MODKEY WLR_MODIFIER_LOGO

#define TAGKEYS(KEY) \
	{ MODKEY,                    XKB_KEY_r, XKB_KEY_##KEY, view,       {.ui = 1 << (XKB_KEY_##KEY - XKB_KEY_a)} }, \
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_R, XKB_KEY_##KEY, tag,        {.ui = 1 << (XKB_KEY_##KEY - XKB_KEY_a)} }, \
	{ MODKEY,                    XKB_KEY_e, XKB_KEY_##KEY, toggletag,  {.ui = 1 << (XKB_KEY_##KEY - XKB_KEY_a)} }, \
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_E, XKB_KEY_##KEY, toggleview, {.ui = 1 << (XKB_KEY_##KEY - XKB_KEY_a)} }


#define CHVT(n) { WLR_MODIFIER_CTRL|WLR_MODIFIER_ALT, -1, XKB_KEY_XF86Switch_VT_##n, chvt, {.ui = (n)} }

#define SPAWN(...) {.v = (const char*[]){ __VA_ARGS__, NULL } }

/* commands */
static const char *termcmd[] = { TERMINAL, NULL };
static const char *menucmd[] = { "run-menu", NULL };

static const char *scratchpadcmd[] = { "k", "keepassxc", NULL };

static const Key keys[] = {
	/* Note that Shift changes certain key codes: c -> C, 2 -> at, etc. */
	/* modifier                  chain, key                 function          argument */
	/* window management */
	{ MODKEY,                    -1,    XKB_KEY_d,               spawn,            {.v = menucmd} },
	{ MODKEY,                    -1,    XKB_KEY_Return,          spawn,            {.v = termcmd} },
	{ MODKEY,                    -1,    XKB_KEY_j,               focusstack,       {.i = +1} },
	{ MODKEY,                    -1,    XKB_KEY_k,               focusstack,       {.i = -1} },
	{ MODKEY,                    -1,    XKB_KEY_y,               movestack,        {.i = -1} },
	{ MODKEY,                    -1,    XKB_KEY_u,               movestack,        {.i = +1} },
	{ MODKEY,                    -1,    XKB_KEY_h,               viewnextocctag,   {.i = -1} },
	{ MODKEY,                    -1,    XKB_KEY_l,               viewnextocctag,   {.i = +1} },
	{ MODKEY|WLR_MODIFIER_SHIFT, -1,    XKB_KEY_H,               setmfact,         {.f = -0.05f} },
	{ MODKEY|WLR_MODIFIER_SHIFT, -1,    XKB_KEY_L,               setmfact,         {.f = +0.05f} },
	{ MODKEY,                    -1,    XKB_KEY_space,           zoom,             {0} },
	{ MODKEY,                    -1,    XKB_KEY_Tab,             view,             {0} },
	{ MODKEY,                    -1,    XKB_KEY_q,               killclient,       {0} },
	{ MODKEY,                    -1,    XKB_KEY_m,               focusmon,         {.i = +1 } },
	{ MODKEY,                    -1,    XKB_KEY_n,               tagmon,           {.i = +1 } },
	{ MODKEY,                    -1,    XKB_KEY_o,               togglescratch,    {.v = scratchpadcmd } },
	{ MODKEY,                    -1,    XKB_KEY_equal,           spawn,            SPAWN("pmenu") },
	{ MODKEY|WLR_MODIFIER_SHIFT, -1,    XKB_KEY_Q,               quit,             {0} },

	/* layouts */
	{ MODKEY,                    XKB_KEY_t,    XKB_KEY_c,        setlayout,        {.v = &layouts[LAYOUT_COLUMN]} },
	{ MODKEY,                    XKB_KEY_t,    XKB_KEY_t,        setlayout,        {.v = &layouts[LAYOUT_TILE]} },
	{ MODKEY,                    XKB_KEY_t,    XKB_KEY_f,        setlayout,        {.v = &layouts[LAYOUT_FLOAT]} },
	{ MODKEY,                    XKB_KEY_t,    XKB_KEY_m,        setlayout,        {.v = &layouts[LAYOUT_MONOCLE]} },
	{ MODKEY,                    XKB_KEY_t,    XKB_KEY_g,        setlayout,        {.v = &layouts[LAYOUT_GAPLESSGRID]} },
	{ MODKEY,                    XKB_KEY_t,    XKB_KEY_x,        togglefullscreen, {0} },
	{ MODKEY,                    XKB_KEY_t,    XKB_KEY_o,        togglefloating,   {0} },

	/* shortcuts */
	{ MODKEY,                    -1,   XKB_KEY_z,                spawn,            SPAWN("wllock", "-z") },
	{ MODKEY|WLR_MODIFIER_SHIFT, -1,   XKB_KEY_Z,                spawn,            SPAWN("wllock", "-s") },
	{ MODKEY,                    -1,   XKB_KEY_w,                spawn,            SPAWN(BROWSER) },
	{ MODKEY|WLR_MODIFIER_SHIFT, -1,   XKB_KEY_W,                spawn,            SPAWN(TERMINAL, "-e", "nmtui") },
	{ MODKEY,                    -1,   XKB_KEY_g,                spawn,            SPAWN("ss", "-s") },
	{ MODKEY|WLR_MODIFIER_SHIFT, -1,   XKB_KEY_G,                spawn,            SPAWN("ss") },
	{ MODKEY,                    -1,   XKB_KEY_b,                spawn,            SPAWN("clipboard-menu") },
	{ MODKEY,                    -1,   XKB_KEY_a,                spawn,            SPAWN("dunstctl", "close-all") },

	/* uniform keybinding magic */
	{ MODKEY,                    -1,   XKB_KEY_c,                spawn,            SPAWN("dwl-keys", "copy") },
	{ MODKEY,                    -1,   XKB_KEY_v,                spawn,            SPAWN("dwl-keys", "paste") },
	{ MODKEY,                    -1,   XKB_KEY_x,                spawn,            SPAWN("dwl-keys", "cut") },
	{ MODKEY,                    -1,   XKB_KEY_BackSpace,        spawn,            SPAWN("dwl-keys", "deleteword") },

	/* media */
	{ MODKEY,                    -1,   XKB_KEY_p,                spawn,            SPAWN("media-command", "playpause") },
	{ 0,                         -1,   XF86XK_AudioPlay,         spawn,            SPAWN("media-command", "playpause") },
	{ MODKEY,                    -1,   XKB_KEY_bracketleft,      spawn,            SPAWN("media-command", "prev") },
	{ 0,                         -1,   XF86XK_AudioPrev,         spawn,            SPAWN("media-command", "prev") },
	{ MODKEY,                    -1,   XKB_KEY_bracketright,     spawn,            SPAWN("media-command", "next") },
	{ 0,                         -1,   XF86XK_AudioNext,         spawn,            SPAWN("media-command", "next") },
	{ WLR_MODIFIER_ALT,          -1,   XKB_KEY_comma,            spawn,            SPAWN("change-volume", "down") },
	{ MODKEY,                    -1,   XKB_KEY_comma,            spawn,            SPAWN("change-volume", "down") },
	{ 0,                         -1,   XF86XK_AudioLowerVolume,  spawn,            SPAWN("change-volume", "down") },
	{ WLR_MODIFIER_ALT,          -1,   XKB_KEY_period,           spawn,            SPAWN("change-volume", "up") },
	{ MODKEY,                    -1,   XKB_KEY_period,           spawn,            SPAWN("change-volume", "up") },
	{ 0,                         -1,   XF86XK_AudioRaiseVolume,  spawn,            SPAWN("change-volume", "up") },
	{ WLR_MODIFIER_ALT,          -1,   XKB_KEY_slash,            spawn,            SPAWN("change-volume", "toggle") },
	{ MODKEY,                    -1,   XKB_KEY_slash,            spawn,            SPAWN("change-volume", "toggle") },
	{ 0,                         -1,   XF86XK_AudioMute,         spawn,            SPAWN("change-volume", "toggle") },
	{ 0,                         -1,   XF86XK_MonBrightnessUp,   spawn,            SPAWN("brighten", "up") },
	{ 0,                         -1,   XF86XK_MonBrightnessDown, spawn,            SPAWN("brighten", "down") },

	/* tags */
	TAGKEYS(a), TAGKEYS(b), TAGKEYS(c), TAGKEYS(d), TAGKEYS(e), TAGKEYS(f),
	TAGKEYS(g), TAGKEYS(h), TAGKEYS(i), TAGKEYS(j), TAGKEYS(k), TAGKEYS(l),
	TAGKEYS(m), TAGKEYS(n), TAGKEYS(o), TAGKEYS(p), TAGKEYS(q), TAGKEYS(r),
	TAGKEYS(s), TAGKEYS(t), TAGKEYS(u), TAGKEYS(v), TAGKEYS(w), TAGKEYS(x),
	TAGKEYS(y), TAGKEYS(z),

	/* virtual terminals */
	CHVT(1), CHVT(2), CHVT(3), CHVT(4), CHVT(5), CHVT(6),
};

static const Button buttons[] = {
	{ MODKEY, BTN_LEFT,   moveresize,     {.ui = CurMove} },
	{ MODKEY, BTN_MIDDLE, moveresize,     {.ui = CurResize} },
};

%module lo
%begin %{
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4244)
#endif
%}
%{
#include "constants.h"
#include "dialog.h"
#include "etc1.h"
#include "extrapolator.h"
#include "field.h"
#include "file.h"
#include "font.h"
#include "image.h"
#include "input.h"
#include "ktx.h"
#include "kvmsg.h"
#include "laidoff.h"
#include "logic.h"
#include "lwanim.h"
#include "lwatlasenum.h"
#include "lwatlassprite.h"
#include "lwattrib.h"
#include "lwbattlecommand.h"
#include "lwbattlecommandresult.h"
#include "lwbattlecreature.h"
#include "lwbattlestate.h"
#include "lwbitmapcontext.h"
#include "lwbox2dcollider.h"
#include "lwbuttoncommand.h"
#include "lwcontext.h"
#include "lwdamagetext.h"
#include "lwdeltatime.h"
#include "lwenemy.h"
#include "lwfbo.h"
#include "lwfieldobject.h"
#include "lwgamescene.h"
#include "lwgl.h"
#include "lwkeyframe.h"
#include "lwlog.h"
#include "lwmacro.h"
#include "lwpkm.h"
#include "lwprogrammedtex.h"
#include "lwshader.h"
#include "lwsimpleanim.h"
#include "lwskill.h"
#include "lwskilleffect.h"
#include "lwskinmesh.h"
#include "lwtextblock.h"
#include "lwtimepoint.h"
#include "lwtouchproc.h"
#include "lwtrail.h"
#include "lwuialign.h"
#include "lwuidim.h"
#include "lwvbo.h"
#include "lwvbotype.h"
#include "mq.h"
#include "nav.h"
#include "net.h"
#include "pcg_basic.h"
#include "platform_detection.h"
#include "playersm.h"
#include "ps.h"
#include "render_admin.h"
#include "render_battle.h"
#include "render_battle_result.h"
#include "render_dialog.h"
#include "render_fan.h"
#include "render_field.h"
#include "render_font_test.h"
#include "render_physics.h"
#include "render_ps.h"
#include "render_skin.h"
#include "render_solid.h"
#include "render_text_block.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "sprite_data.h"
#include "sysmsg.h"
#include "tex.h"
#include "tinyobj_loader_c.h"
#include "unicode.h"
#include "vertices.h"
#include "zhelpers.h"
#include "armature.h"
#include "battle.h"
#include "battle_result.h"
#include "battlelogic.h"
#include "rmsg.h"
#include "lwparabola.h"
#include "construct.h"
#include "puckgameupdate.h"
#include "puckgame.h"
#include "lwtcp.h"
#include "lwtcpclient.h"
#include "puckgamepacket.h"
#include "htmlui.h"
#include "lwttl.h"
#ifdef WIN32
#pragma warning(pop)
#endif
%}

%ignore s_set_id;
%ignore _LWTIMEPOINT;
%ignore BMF_CHAR;

%include "../glfw-3.2.1/deps/linmath.h"
%include "lwmacro.h"
%include "constants.h"
%include "dialog.h"
%include "extrapolator.h"
%include "field.h"
%include "file.h"
%include "font.h"
%include "input.h"
%include "ktx.h"
%include "kvmsg.h"
%include "laidoff.h"
%include "logic.h"
%include "lwanim.h"
%include "lwatlasenum.h"
%include "lwatlassprite.h"
%include "lwattrib.h"
%include "lwbattlecommand.h"
%include "lwbattlecommandresult.h"
%include "lwbattlecreature.h"
%include "lwbattlestate.h"
%include "lwbitmapcontext.h"
%include "lwbox2dcollider.h"
%include "lwbuttoncommand.h"
%include "lwcontext.h"
%include "lwdamagetext.h"
%include "lwdeltatime.h"
%include "lwenemy.h"
%include "lwfbo.h"
%include "lwfieldobject.h"
%include "lwgamescene.h"
%include "lwgl.h"
%include "lwkeyframe.h"
%include "lwlog.h"
%include "lwpkm.h"
%include "lwprogrammedtex.h"
%include "lwshader.h"
%include "lwsimpleanim.h"
%include "lwskill.h"
%include "lwskilleffect.h"
%include "lwskinmesh.h"
%include "lwtextblock.h"
%include "lwtimepoint.h"
%include "lwtouchproc.h"
%include "lwtrail.h"
%include "lwuialign.h"
%include "lwuidim.h"
%include "lwvbo.h"
%include "lwvbotype.h"
%include "mq.h"
%include "nav.h"
%include "net.h"
%include "pcg_basic.h"
%include "platform_detection.h"
%include "playersm.h"
%include "ps.h"
%include "render_admin.h"
%include "render_battle.h"
%include "render_battle_result.h"
%include "render_dialog.h"
%include "render_fan.h"
%include "render_field.h"
%include "render_font_test.h"
%include "render_physics.h"
%include "render_ps.h"
%include "render_skin.h"
%include "render_solid.h"
%include "render_text_block.h"
%include "script.h"
%include "sound.h"
%include "sprite.h"
%include "sprite_data.h"
%include "sysmsg.h"
%include "tex.h"
%include "tinyobj_loader_c.h"
%include "unicode.h"
%include "vertices.h"
%include "zhelpers.h"
%include "armature.h"
%include "battle.h"
%include "battle_result.h"
%include "battlelogic.h"
%include "rmsg.h"
%include "lwparabola.h"
%include "construct.h"
%include "puckgameupdate.h"
%include "puckgame.h"
%include "lwtcp.h"
%include "lwtcpclient.h"
%include "puckgamepacket.h"
%include "htmlui.h"
%include "lwttl.h"

// using the C-array
%include <carrays.i>
%array_functions(int,int)
%array_functions(float,float)
%array_functions(LWPUCKGAMEOBJECT,PUCKGAMEOBJECT)
%array_functions(LWPUCKGAMETOWER,PUCKGAMETOWER)

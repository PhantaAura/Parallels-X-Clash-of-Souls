#!/usr/bin/env python3
"""Author Chapter-1 Rrvvfo animation clips from Legacy sprite timing/key poses.

The Legacy atlases remain the pose/timing authority. This sidecar translates
those 2D silhouettes to Rrvvfo's unchanged 39-joint deform rig; the engine
interpolates between authored keys. It deliberately avoids gameplay code
containing character choreography.
"""
from __future__ import annotations
import argparse, json, math, runpy
from pathlib import Path

HERE = Path(__file__).resolve().parent
legacy_idle = runpy.run_path(str(HERE / "author-rrvvfo-idle.py"))
normalize=legacy_idle["normalize"]; multiply=legacy_idle["multiply"]; inverse=legacy_idle["inverse"]
rotate=legacy_idle["rotate"]; from_to=legacy_idle["from_to"]; delta_rotation=legacy_idle["delta_rotation"]
parse_glb=legacy_idle["parse_glb"]

FIST_L=("Middle.L","Pinky.L","Ring.L","Upper.L")
FIST_R=("hand.R.001","hand.R.002","Middle.R","Pinky.R","Ring.R","Upper.R")
REQUIRED={
    "spine","pelvis.L","pelvis.R","spine.001","spine.002","spine.003",
    "shoulder.L","upper_arm.L","forearm.L","hand.L",*FIST_L,"Thumb.L",
    "shoulder.R","upper_arm.R","forearm.R","hand.R",*FIST_R,"Thumb.R",
    "spine.004","spine.005","spine.006",
    "thigh.L","shin.L","foot.L","thigh.R","shin.R","foot.R",
}
ORDER=[
    "spine","pelvis.L","pelvis.R","spine.001","spine.002","spine.003",
    "shoulder.L","upper_arm.L","forearm.L","hand.L","Middle.L","Pinky.L","Ring.L","Thumb.L","Upper.L",
    "shoulder.R","upper_arm.R","forearm.R","hand.R","hand.R.001","hand.R.002","Middle.R","Pinky.R","Ring.R","Thumb.R","Upper.R",
    "spine.004","spine.005","spine.006","thigh.L","shin.L","foot.L","thigh.R","shin.R","foot.R",
]

# Character-space defaults. +Y is up and +Z is forward.
BASE={
 "yaw":0.0,"root":0.0,"lean":0.0,"pitch":0.0,"head":0.0,"fist":68.0,
 "lu":[.19,-.92,.34],"lf":[.05,-.97,.24],"ru":[-.19,-.92,.34],"rf":[-.05,-.97,.24],
 "lt":[.18,-.97,.16],"ls":[.10,-.99,-.05],"rt":[-.18,-.97,.16],"rs":[-.10,-.99,-.05],
}
def P(**kw):
    d={k:(list(v) if isinstance(v,list) else v) for k,v in BASE.items()}; d.update(kw); return d

def stance(phase=0):
    # Legacy stance: broad base, shoulders turned, fists forward rather than hanging.
    bob=[0.0,.006,-.004,.003][phase%4]
    return P(yaw=8+[-2,0,2,0][phase%4],root=bob,lean=2,pitch=4,head=-4,
             lu=[.23,-.55,.80],lf=[.05,-.18,.98],ru=[-.28,-.62,.73],rf=[-.06,-.26,.96],
             lt=[.20,-.95,.23],ls=[.10,-.99,-.05],rt=[-.25,-.94,.22],rs=[-.12,-.99,-.04])

def run_pose(front_left: bool, high: bool=False):
    s=1 if front_left else -1
    return P(yaw=3*s,root=.012 if high else -.006,lean=-2*s,pitch=13,head=-2*s,
             lu=[.15*s,-.42,.89 if s>0 else -.89], lf=[.05*s,-.72,.69 if s>0 else -.69],
             ru=[-.15*s,-.42,-.89 if s>0 else .89], rf=[-.05*s,-.72,-.69 if s>0 else .69],
             lt=[.16,-.70,.70*s], ls=[.08,-.90,.42*s], rt=[-.16,-.70,-.70*s], rs=[-.08,-.90,-.42*s])

def dash_pose(k):
    # Four Legacy dash frames: compressed launch -> long forward silhouette -> recovery.
    if k==0: return P(root=-.018,pitch=20,lean=0,lu=[.14,-.42,-.89],lf=[.04,-.55,-.83],ru=[-.14,-.42,-.89],rf=[-.04,-.55,-.83],lt=[.14,-.86,.49],ls=[.07,-.96,.26],rt=[-.14,-.72,-.68],rs=[-.07,-.94,-.33])
    if k==1: return P(root=.006,pitch=29,lu=[.12,-.30,-.95],lf=[.03,-.36,-.93],ru=[-.12,-.30,-.95],rf=[-.03,-.36,-.93],lt=[.12,-.69,.71],ls=[.06,-.88,.47],rt=[-.12,-.66,-.74],rs=[-.06,-.88,-.47])
    if k==2: return P(root=.010,pitch=32,lu=[.10,-.24,-.97],lf=[.02,-.31,-.95],ru=[-.10,-.24,-.97],rf=[-.02,-.31,-.95],lt=[.11,-.61,.78],ls=[.05,-.83,.55],rt=[-.11,-.60,-.79],rs=[-.05,-.84,-.54])
    return P(root=-.010,pitch=17,lu=[.14,-.46,-.88],lf=[.04,-.62,-.78],ru=[-.14,-.46,-.88],rf=[-.04,-.62,-.78],lt=[.16,-.88,.44],ls=[.08,-.97,.22],rt=[-.16,-.78,-.61],rs=[-.08,-.95,-.30])

def jump_pose(kind):
    if kind=="start": return P(root=-.035,pitch=9,lu=[.18,-.54,.82],lf=[.05,-.27,.96],ru=[-.18,-.54,.82],rf=[-.05,-.27,.96],lt=[.18,-.72,.67],ls=[.10,-.76,-.64],rt=[-.18,-.72,.67],rs=[-.10,-.76,-.64])
    if kind=="rise": return P(root=.015,pitch=4,lu=[.24,-.32,.92],lf=[.08,-.20,.98],ru=[-.24,-.32,.92],rf=[-.08,-.20,.98],lt=[.17,-.78,.60],ls=[.09,-.84,-.53],rt=[-.17,-.78,.60],rs=[-.09,-.84,-.53])
    if kind=="fall": return P(root=.006,pitch=-3,lu=[.28,-.66,.70],lf=[.10,-.79,.60],ru=[-.28,-.66,.70],rf=[-.10,-.79,.60],lt=[.18,-.88,.43],ls=[.09,-.96,-.27],rt=[-.18,-.88,.43],rs=[-.09,-.96,-.27])
    return P(root=-.025,pitch=7,lu=[.20,-.62,.76],lf=[.06,-.44,.90],ru=[-.20,-.62,.76],rf=[-.06,-.44,.90],lt=[.20,-.82,.54],ls=[.10,-.93,-.36],rt=[-.20,-.82,.54],rs=[-.10,-.93,-.36])

def attack_pose(side, amount, vertical=0.0, windup=False):
    # Legacy normals turn the body into strikes. Amount controls extension.
    sign=1 if side=="L" else -1
    p=stance(0); p["yaw"]=-18*sign if windup else 23*sign; p["pitch"]=8 if windup else 12; p["lean"]=-4*sign
    forward=max(.2,min(.99,amount)); up=vertical
    arm=[.20*sign, up, forward]; fore=[.06*sign, up*.45, .995]
    if windup: arm=[.22*sign,-.50,-.84]; fore=[.08*sign,-.40,-.91]
    if side=="L": p["lu"]=arm; p["lf"]=fore
    else: p["ru"]=arm; p["rf"]=fore
    return p

def heavy_pose(stage):
    if stage==0: return P(yaw=-24,root=-.010,pitch=13,lu=[.18,-.45,-.87],lf=[.05,-.32,-.95],ru=[-.18,-.57,.80],rf=[-.05,-.30,.95])
    if stage==1: return P(yaw=30,root=.004,pitch=18,lu=[.18,-.32,.93],lf=[.05,-.10,.99],ru=[-.20,-.18,.96],rf=[-.06,-.05,.998])
    return stance(1)

def launcher_pose(stage):
    if stage==0: return P(yaw=-8,root=-.030,pitch=14,ru=[-.18,-.55,-.81],rf=[-.06,-.40,-.91],lt=[.16,-.77,.62],rt=[-.16,-.77,.62])
    if stage==1: return P(yaw=12,root=.012,pitch=18,ru=[-.15,.58,.80],rf=[-.05,.78,.62],lu=[.18,-.38,.91],lf=[.05,-.14,.99],lt=[.14,-.86,.49],rt=[-.14,-.86,.49])
    return stance(2)

def guard_pose(perfect=False):
    return P(yaw=12,root=-.006,pitch=5,lu=[.25,-.30,.92],lf=[.08,.20,.98],ru=[-.25,-.34,.90],rf=[-.08,.15,.985],
             lt=[.20,-.95,.23],rt=[-.24,-.94,.24], head=-5 if perfect else -2)

def hurt_pose():
    return P(yaw=-18,root=-.008,pitch=-13,lean=-6,lu=[.28,-.64,-.72],lf=[.10,-.74,-.66],ru=[-.28,-.58,-.76],rf=[-.10,-.72,-.68],lt=[.18,-.91,.37],rt=[-.20,-.91,.36])

def power_pose(stage):
    if stage==0: return stance(0)
    if stage==1: return P(yaw=0,root=-.008,pitch=2,lu=[.34,-.18,.92],lf=[.12,.05,.99],ru=[-.34,-.18,.92],rf=[-.12,.05,.99],lt=[.19,-.96,.20],rt=[-.19,-.96,.20])
    return P(yaw=0,root=.006,pitch=-2,lu=[.38,.10,.92],lf=[.14,.22,.96],ru=[-.38,.10,.92],rf=[-.14,.22,.96],lt=[.19,-.96,.20],rt=[-.19,-.96,.20])

def beam_pose(stage):
    # Used for Object Swap because Legacy's swap startup/disappear/reappear uses beam_01..04.
    if stage==0: return stance(0)
    if stage==1: return P(yaw=10,root=-.012,pitch=8,lu=[.16,-.38,.91],lf=[.05,-.10,.99],ru=[-.24,-.34,.91],rf=[-.08,-.04,.996])
    if stage==2: return P(yaw=15,root=.010,pitch=12,lu=[.12,-.24,.96],lf=[.04,.04,.998],ru=[-.28,-.26,.92],rf=[-.08,.04,.996])
    return stance(1)

def fire_pose(stage):
    if stage==0: return P(yaw=-16,root=-.012,pitch=12,ru=[-.22,-.44,-.87],rf=[-.07,-.32,-.94],lu=[.18,-.50,.85],lf=[.05,-.22,.97])
    if stage==1: return P(yaw=18,root=.002,pitch=12,ru=[-.16,-.14,.98],rf=[-.05,-.02,.999],lu=[.14,-.48,.86],lf=[.04,-.20,.98])
    return stance(0)

def build_rig(document):
    nodes=document["nodes"]; joint_nodes=document["skins"][0]["joints"]
    joint_lookup={node:i for i,node in enumerate(joint_nodes)}
    names={nodes[node].get("name",f"joint_{i}"):i for i,node in enumerate(joint_nodes)}
    missing=sorted(REQUIRED.difference(names))
    if missing: raise ValueError("Missing deform joints: "+", ".join(missing))
    parents=[-1]*len(nodes)
    for parent,node in enumerate(nodes):
        for child in node.get("children",[]): parents[child]=parent
    local_bind=[normalize(nodes[n].get("rotation",[0,0,0,1])) for n in joint_nodes]
    trans_bind=[list(nodes[n].get("translation",[0,0,0])) for n in joint_nodes]
    joint_parents=[]
    for n in joint_nodes:
        parent=parents[n]
        while parent>=0 and parent not in joint_lookup: parent=parents[parent]
        joint_parents.append(joint_lookup.get(parent,-1))
    return names,local_bind,trans_bind,joint_parents

def sample_pose(pose,names,local_bind,trans_bind,joint_parents):
    local=[list(q) for q in local_bind]; world=[None]*len(local)
    def refresh():
        for i in range(len(local)):
            par=joint_parents[i]; world[i]=local[i] if par<0 else multiply(world[par],local[i])
    refresh()
    def add(name,*axes):
        i=names[name]; local[i]=multiply(local_bind[i],delta_rotation(*axes)); refresh()
    def aim(name,target):
        i=names[name]; par=joint_parents[i]
        current=rotate(world[i],[0,1,0]); desired=multiply(from_to(current,target),world[i])
        local[i]=desired if par<0 else multiply(inverse(world[par]),desired); refresh()
    add("spine",([0,1,0],pose["yaw"]),([0,0,1],pose["lean"]*.30))
    add("spine.001",([0,0,1],pose["lean"]*.36),([1,0,0],pose["pitch"]*.32))
    add("spine.002",([0,0,1],pose["lean"]*.42),([1,0,0],pose["pitch"]*.38))
    add("spine.003",([0,0,1],pose["lean"]*.24),([1,0,0],pose["pitch"]*.30))
    add("pelvis.L",([0,0,1],-pose["lean"]*.22)); add("pelvis.R",([0,0,1],-pose["lean"]*.22))
    add("shoulder.L",([0,0,1],-3-pose["lean"]*.55)); add("shoulder.R",([0,0,1],1.5-pose["lean"]*.30))
    for side in ("L","R"):
        aim(f"upper_arm.{side}",pose["lu" if side=="L" else "ru"])
        aim(f"forearm.{side}",pose["lf" if side=="L" else "rf"])
        f=pose["lf" if side=="L" else "rf"]
        aim(f"hand.{side}",normalize([f[0]*.45,f[1],f[2]]))
    for finger in FIST_L: add(finger,([1,0,0],pose["fist"]))
    add("Thumb.L",([0,0,1],42),([1,0,0],24))
    for finger in FIST_R: add(finger,([1,0,0],pose["fist"]))
    add("Thumb.R",([0,0,1],-42),([1,0,0],24))
    aim("thigh.L",pose["lt"]); aim("shin.L",pose["ls"]); aim("foot.L",[.08,-.52,.85])
    aim("thigh.R",pose["rt"]); aim("shin.R",pose["rs"]); aim("foot.R",[-.08,-.52,.85])
    add("spine.004",([0,1,0],pose["head"]*.42),([1,0,0],-pose["pitch"]*.18))
    add("spine.005",([0,1,0],pose["head"]*.35),([0,0,1],-pose["lean"]*.16))
    add("spine.006",([0,1,0],pose["head"]*.23))
    root=list(trans_bind[names["spine"]]); root[1]+=pose["root"]
    return local,root

def clip(name,duration,loop,poses,times,names,local_bind,trans_bind,joint_parents,source):
    assert len(poses)==len(times)
    rotations={n:[] for n in ORDER}; roots=[]
    for p in poses:
        local,root=sample_pose(p,names,local_bind,trans_bind,joint_parents)
        for n in ORDER: rotations[n].append(local[names[n]])
        roots.append(root)
    tracks=[]
    for n in ORDER:
        tr={"joint":n,"interpolation":"LINEAR","rotation":[{"time":round(t,6),"value":v} for t,v in zip(times,rotations[n])]}
        if n=="spine": tr["translation"]=[{"time":round(t,6),"value":v} for t,v in zip(times,roots)]
        tracks.append(tr)
    return {"name":name,"duration":duration,"loop":loop,"source":source,"tracks":tracks}

def evenly(duration,poses,loop=False):
    if len(poses)==1: return [0.0]
    if loop:
        # Last pose is closure copy at exact duration.
        return [duration*i/(len(poses)-1) for i in range(len(poses))]
    return [duration*i/(len(poses)-1) for i in range(len(poses))]

def main():
    ap=argparse.ArgumentParser(); ap.add_argument("source",type=Path); ap.add_argument("idle",type=Path); ap.add_argument("output",type=Path); a=ap.parse_args()
    doc=parse_glb(a.source); names,bind,tbind,parents=build_rig(doc)
    idle=json.loads(a.idle.read_text())
    clips=list(idle["clips"])
    def add(name,dur,loop,poses,source,times=None):
        if loop and poses[-1] is not poses[0]: poses=list(poses)+[poses[0]]
        if times is None: times=evenly(dur,poses,loop)
        clips.append(clip(name,dur,loop,poses,times,names,bind,tbind,parents,source))

    add("fighting_stance",.420,True,[stance(i) for i in range(4)],"Legacy fightingStance stance_01..04 @105ms")
    add("run",.328,True,[run_pose(True),run_pose(False,True),run_pose(False),run_pose(True,True)],"Legacy run_01..04 @82ms")
    add("dash",.240,False,[dash_pose(i) for i in range(4)],"Legacy dash_01..04 @60ms")
    add("jump_start",.150,False,[jump_pose("start"),jump_pose("rise")],"Legacy jump_01..02 @75ms")
    add("fall",.220,True,[jump_pose("rise"),jump_pose("fall")],"Legacy jump_03..04 @110ms")
    add("land",.130,False,[jump_pose("land"),stance(0)],"Legacy jump_05 + stance_01 @65ms")
    add("light_1",.270,False,[stance(0),attack_pose("R",.65,windup=True),attack_pose("R",.99),stance(1)],"Legacy light1 attack_01..02; gameplay duration .27s")
    add("light_2",.290,False,[stance(1),attack_pose("L",.60,windup=True),attack_pose("L",.99),stance(2)],"Legacy light2 attack_02..03; gameplay duration .29s")
    add("light_3",.390,False,[stance(2),heavy_pose(0),attack_pose("R",.99,vertical=.18),stance(0)],"Legacy light3 attack_03..04; gameplay duration .39s")
    add("heavy",.620,False,[stance(0),heavy_pose(0),heavy_pose(1),stance(1)],"Legacy heavy startup/active/recovery; gameplay duration .62s")
    add("launcher",.550,False,[stance(0),launcher_pose(0),launcher_pose(1),stance(2)],"Legacy launcher startup/active/recovery; gameplay duration .55s")
    add("air_light",.360,False,[jump_pose("rise"),attack_pose("R",.99,vertical=.05),jump_pose("fall")],"Legacy air attack key silhouettes; gameplay duration .36s")
    add("air_heavy",.520,False,[jump_pose("rise"),heavy_pose(1),jump_pose("fall")],"Legacy air heavy key silhouettes; gameplay duration .52s")
    add("pursuit_light",.320,False,[dash_pose(2),attack_pose("R",.99),jump_pose("fall")],"Legacy pursuit uses dash/attack vocabulary; gameplay duration .32s")
    add("pursuit_heavy",.460,False,[dash_pose(2),heavy_pose(1),jump_pose("fall")],"Legacy pursuit finisher vocabulary; gameplay duration .46s")
    add("grab",.380,False,[stance(0),attack_pose("L",.92),stance(1)],"Legacy combat grab timing; shared authored pose translation")
    add("block",.240,True,[guard_pose(False),guard_pose(True)],"Legacy blockHold block_02 @120ms")
    add("perfect_block",.140,False,[guard_pose(False),guard_pose(True)],"Legacy perfectBlock block_03..04 @70ms")
    add("hurt",.210,False,[hurt_pose(),stance(0)],"Legacy hurtHeavy hurt_02..03 @105ms")
    add("charge",.276,True,[power_pose(0),power_pose(1),power_pose(2)],"Legacy chargeEnergy ultimate_01..03 @92ms")
    add("counter",.216,False,[guard_pose(False),guard_pose(True),heavy_pose(1)],"Legacy counter block_01/perfect_01/heavy_02 @72ms")
    add("breaker",.150,False,[stance(0),heavy_pose(1)],"Legacy breaker stance_01/heavy_02 @75ms")
    add("fire_blast",.420,False,[fire_pose(0),fire_pose(1),fire_pose(2)],"Legacy Rrvvfo fire/projectile pose vocabulary; gameplay duration .42s")
    add("object_swap",.320,False,[beam_pose(i) for i in range(4)],"Legacy Object Swap beam_01..04 @60-80ms translated to .32s")
    add("lens_activate",.300,False,[power_pose(i) for i in range(3)],"Legacy Lens power_01..03 @80ms translated to .30s")
    payload={"version":1,"source":"Legacy 2.9A.40.7.1.1 Rrvvfo atlas key poses/timings translated to unchanged 39-joint rig","clips":clips}
    a.output.parent.mkdir(parents=True,exist_ok=True); a.output.write_text(json.dumps(payload,indent=2)+"\n")
    print(f"Authored {len(clips)} Chapter-1 Rrvvfo clips -> {a.output}")

if __name__=="__main__": main()

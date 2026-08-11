#!/usr/bin/env python3
"""Author Omega Rrvvfo animation clips from the Legacy sprite-sheet language.

The Legacy/Sprite references remain the pose and timing authority, but Omega
translates the silhouettes into poses that read naturally on Rrvvfo's unchanged
39-joint 3D deform rig. Impossible 2D smear/stretch is intentionally represented
by cleaner keyed poses and is left to renderer VFX rather than breaking anatomy.

Hard character rule:
- `idle` is exploration/hub only.
- active combat settles into `fighting_stance`.
- combat movement uses dedicated `combat_advance` / `combat_retreat` clips.
"""
from __future__ import annotations
import argparse, json, runpy
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


def legacy_idle_pose(index=0):
    """Use an actual authored Legacy idle silhouette as the Story transition boundary."""
    pose=legacy_idle["POSES"][index % len(legacy_idle["POSES"])]
    return P(yaw=pose["root_yaw"],root=pose["root_height"],lean=pose["torso_lean"],
             pitch=pose["torso_pitch"],head=pose["head_yaw"],
             lu=list(pose["left_upper"]),lf=list(pose["left_fore"]),
             ru=list(pose["right_upper"]),rf=list(pose["right_fore"]),
             lt=list(pose["left_thigh"]),ls=list(pose["left_shin"]),
             rt=list(pose["right_thigh"]),rs=list(pose["right_shin"]))


def run_start_pose(stage: int):
    """Story-only launch: exact Legacy idle boundary -> compression -> sprint contact."""
    if stage == 0: return legacy_idle_pose(3)
    if stage == 1:
        return P(yaw=-2,root=-.030,lean=.5,pitch=18,head=-1,
                 lu=[.15,-.50,-.85],lf=[.05,-.66,-.75],ru=[-.15,-.48,.86],rf=[-.05,-.64,.77],
                 lt=[.18,-.78,.60],ls=[.09,-.97,.22],rt=[-.18,-.68,-.71],rs=[-.09,-.90,-.42])
    return run_pose(0)


def run_stop_pose(stage: int):
    """Story-only brake: sprint contact -> planted deceleration -> Legacy idle boundary."""
    if stage == 0: return run_pose(3)
    if stage == 1:
        return P(yaw=2,root=-.022,lean=-.5,pitch=12,head=0,
                 lu=[.16,-.65,.74],lf=[.05,-.78,.62],ru=[-.16,-.58,-.80],rf=[-.05,-.72,-.69],
                 lt=[.19,-.88,-.43],ls=[.09,-.97,-.22],rt=[-.19,-.91,.36],rs=[-.09,-.99,-.08])
    return legacy_idle_pose(3)


def stance(phase=0):
    """Combat idle: compact, asymmetrical, alert and lightly bouncing."""
    bob=[-.004,.007,-.003,.004][phase%4]
    yaw=[6,9,11,8][phase%4]
    return P(yaw=yaw,root=bob,lean=2.5,pitch=7.0,head=-4.5,
             lu=[.24,-.50,.83],lf=[.07,-.15,.986],
             ru=[-.30,-.57,.76],rf=[-.08,-.20,.977],
             lt=[.23,-.93,.28],ls=[.10,-.995,-.03],
             rt=[-.28,-.92,.27],rs=[-.12,-.992,-.04])


def combat_ready_pose(stage):
    if stage == 0:
        # Meet Story on an actual Legacy idle boundary instead of a generic neutral.
        return legacy_idle_pose(3)
    if stage == 1:
        # Plant the rear foot and pull the hands up.
        return P(yaw=4,root=-.014,pitch=8,lean=2,head=-3,
                 lu=[.22,-.62,.75],lf=[.06,-.30,.952],
                 ru=[-.25,-.67,.70],rf=[-.07,-.34,.938],
                 lt=[.22,-.91,.34],ls=[.10,-.99,-.03],
                 rt=[-.26,-.90,.34],rs=[-.12,-.99,-.04])
    return stance(0)


def combat_relax_pose(stage):
    if stage == 0: return stance(2)
    if stage == 1:
        return P(yaw=3,root=-.003,pitch=3,head=-1,
                 lu=[.20,-.72,.66],lf=[.05,-.72,.69],
                 ru=[-.20,-.75,.63],rf=[-.05,-.74,.67],
                 lt=[.20,-.96,.19],rt=[-.20,-.96,.19])
    return legacy_idle_pose(3)


def run_pose(phase: int):
    """Exploration sprint keyed directly around the four Legacy run silhouettes.

    The sprite sheet reads as contact -> compressed drive -> airborne extension ->
    opposite contact. 3D keeps legal anatomy, but the torso angle, stride length,
    arm drive and flight beat are intentionally exaggerated so this cannot read
    as a casual walk at gameplay distance.
    """
    phase %= 4
    poses = [
        # RUN 1: planted drive. Rear leg pushes, lead knee comes through.
        P(yaw=-3,root=-.018,lean=1,pitch=26,head=-2,
          lu=[.15,-.28,-.95],lf=[.05,-.43,-.90],ru=[-.15,-.36,.92],rf=[-.05,-.57,.82],
          lt=[.18,-.58,.79],ls=[.09,-.88,.46],rt=[-.18,-.80,-.58],rs=[-.09,-.96,-.27]),
        # RUN 2: low acceleration/compression, matching the sprite's crouched burst.
        P(yaw=-1,root=-.038,lean=0,pitch=34,head=-1,
          lu=[.14,-.40,-.91],lf=[.04,-.61,-.79],ru=[-.14,-.25,.96],rf=[-.04,-.43,.90],
          lt=[.18,-.76,.62],ls=[.09,-.98,.18],rt=[-.18,-.55,-.81],rs=[-.09,-.84,-.53]),
        # RUN 3: the distinctive long airborne Legacy silhouette.
        P(yaw=2,root=.034,lean=-1,pitch=39,head=1,
          lu=[.12,-.22,-.97],lf=[.03,-.35,-.94],ru=[-.12,-.25,.96],rf=[-.03,-.40,.91],
          lt=[.16,-.49,-.86],ls=[.07,-.79,-.61],rt=[-.16,-.52,.84],rs=[-.07,-.81,.58]),
        # RUN 4: opposite foot catches the body and immediately drives again.
        P(yaw=3,root=-.014,lean=-1,pitch=29,head=2,
          lu=[.15,-.35,.92],lf=[.05,-.56,.82],ru=[-.15,-.28,-.95],rf=[-.05,-.43,-.90],
          lt=[.18,-.81,-.56],ls=[.09,-.96,-.26],rt=[-.18,-.58,.79],rs=[-.09,-.88,.46]),
    ]
    return poses[phase]


def combat_advance_pose(phase: int):
    """Forward combat locomotion keeps the torso guarded and opponent-ready."""
    p = run_pose(phase)
    p["pitch"] = 17 + (phase in (1,4)) * 2
    p["yaw"] *= .55
    p["lean"] *= .5
    # Hands stay closer to guard than the hub sprint's full arm pump.
    if phase in (0,1,5):
        p["lu"]=[.24,-.48,.84]; p["lf"]=[.07,-.18,.98]
    else:
        p["ru"]=[-.27,-.52,.81]; p["rf"]=[-.07,-.20,.98]
    return p


def combat_retreat_pose(phase: int):
    """Purpose-built backpedal: feet retreat while chest/guard stay on opponent."""
    phase %= 4
    poses = [
        P(yaw=7,root=-.010,pitch=8,head=-4,
          lu=[.24,-.49,.84],lf=[.07,-.17,.983],ru=[-.29,-.55,.78],rf=[-.08,-.22,.972],
          lt=[.22,-.86,-.46],ls=[.10,-.98,-.16],rt=[-.27,-.95,.20],rs=[-.12,-.99,-.03]),
        P(yaw=9,root=.008,pitch=7,head=-5,
          lu=[.25,-.48,.84],lf=[.07,-.16,.984],ru=[-.30,-.54,.78],rf=[-.08,-.21,.974],
          lt=[.22,-.94,.24],ls=[.10,-.99,-.04],rt=[-.27,-.83,-.48],rs=[-.12,-.97,-.20]),
        P(yaw=8,root=-.012,pitch=8,head=-4,
          lu=[.25,-.50,.83],lf=[.07,-.18,.981],ru=[-.29,-.53,.80],rf=[-.08,-.20,.976],
          lt=[.22,-.95,.21],ls=[.10,-.99,-.03],rt=[-.27,-.86,-.43],rs=[-.12,-.98,-.16]),
        P(yaw=6,root=.007,pitch=7,head=-4,
          lu=[.24,-.50,.83],lf=[.07,-.18,.981],ru=[-.29,-.54,.79],rf=[-.08,-.21,.974],
          lt=[.22,-.84,-.49],ls=[.10,-.97,-.22],rt=[-.27,-.94,.24],rs=[-.12,-.99,-.04]),
    ]
    return poses[phase]


def dash_pose(k):
    # Compressed launch -> long sprite-like silhouette -> snap recovery.
    if k==0: return P(root=-.024,pitch=24,lean=0,lu=[.14,-.42,-.89],lf=[.04,-.55,-.83],ru=[-.14,-.42,-.89],rf=[-.04,-.55,-.83],lt=[.14,-.86,.49],ls=[.07,-.96,.26],rt=[-.14,-.72,-.68],rs=[-.07,-.94,-.33])
    if k==1: return P(root=.010,pitch=34,lu=[.12,-.27,-.96],lf=[.03,-.34,-.94],ru=[-.12,-.27,-.96],rf=[-.03,-.34,-.94],lt=[.12,-.65,.75],ls=[.06,-.86,.51],rt=[-.12,-.62,-.78],rs=[-.06,-.86,-.50])
    if k==2: return P(root=.012,pitch=36,lu=[.10,-.20,-.975],lf=[.02,-.28,-.96],ru=[-.10,-.20,-.975],rf=[-.02,-.28,-.96],lt=[.11,-.57,.81],ls=[.05,-.80,.59],rt=[-.11,-.56,-.82],rs=[-.05,-.82,-.57])
    return P(root=-.014,pitch=19,lu=[.14,-.46,-.88],lf=[.04,-.62,-.78],ru=[-.14,-.46,-.88],rf=[-.04,-.62,-.78],lt=[.16,-.88,.44],ls=[.08,-.97,.22],rt=[-.16,-.78,-.61],rs=[-.08,-.95,-.30])


def jump_pose(kind):
    if kind=="start": return P(root=-.042,pitch=12,lu=[.18,-.54,.82],lf=[.05,-.27,.96],ru=[-.18,-.54,.82],rf=[-.05,-.27,.96],lt=[.18,-.69,.70],ls=[.10,-.73,-.67],rt=[-.18,-.69,.70],rs=[-.10,-.73,-.67])
    if kind=="rise": return P(root=.020,pitch=6,lu=[.24,-.30,.92],lf=[.08,-.18,.98],ru=[-.24,-.30,.92],rf=[-.08,-.18,.98],lt=[.17,-.75,.64],ls=[.09,-.82,-.56],rt=[-.17,-.75,.64],rs=[-.09,-.82,-.56])
    if kind=="fall": return P(root=.006,pitch=-5,lu=[.28,-.66,.70],lf=[.10,-.79,.60],ru=[-.28,-.66,.70],rf=[-.10,-.79,.60],lt=[.18,-.88,.43],ls=[.09,-.96,-.27],rt=[-.18,-.88,.43],rs=[-.09,-.96,-.27])
    return P(root=-.034,pitch=10,lu=[.20,-.62,.76],lf=[.06,-.44,.90],ru=[-.20,-.62,.76],rf=[-.06,-.44,.90],lt=[.20,-.80,.56],ls=[.10,-.91,-.40],rt=[-.20,-.80,.56],rs=[-.10,-.91,-.40])


def hard_land_pose(stage, combat=False):
    if stage == 0:
        return P(root=-.060,pitch=18,lean=3,head=-6,
                 lu=[.27,-.70,.66],lf=[.10,-.54,.84],ru=[-.22,-.58,.78],rf=[-.07,-.39,.92],
                 lt=[.25,-.66,.71],ls=[.11,-.68,-.72],rt=[-.28,-.72,.63],rs=[-.12,-.76,-.64])
    if stage == 1: return jump_pose("land")
    return stance(0) if combat else legacy_idle_pose(3)


def attack_pose(side, amount, vertical=0.0, windup=False):
    sign=1 if side=="L" else -1
    p=stance(0); p["yaw"]=-20*sign if windup else 26*sign; p["pitch"]=9 if windup else 14; p["lean"]=-5*sign
    forward=max(.2,min(.99,amount)); up=vertical
    arm=[.20*sign, up, forward]; fore=[.06*sign, up*.45, .995]
    if windup: arm=[.22*sign,-.50,-.84]; fore=[.08*sign,-.40,-.91]
    if side=="L": p["lu"]=arm; p["lf"]=fore
    else: p["ru"]=arm; p["rf"]=fore
    return p


def light1_pose(stage):
    # Legacy Light Combo frames 1-2: compact guard -> straight flaming jab.
    if stage == 0: return stance(0)
    if stage == 1:
        return P(yaw=-8,root=-.012,pitch=8,lean=-2,head=-2,
                 ru=[-.22,-.47,.85],rf=[-.07,-.26,.96],lu=[.25,-.51,.82],lf=[.08,-.20,.98],
                 lt=[.22,-.94,.24],rt=[-.25,-.91,.32])
    if stage == 2:
        return P(yaw=18,root=.002,pitch=12,lean=3,head=-4,
                 ru=[-.08,-.08,.994],rf=[-.02,-.01,.999],lu=[.28,-.53,.80],lf=[.09,-.25,.965],
                 lt=[.22,-.95,.20],rt=[-.24,-.91,.34])
    return stance(1)


def light2_pose(stage):
    # Legacy frames 3-4: cross-body fire strike into the long stepping kick silhouette.
    if stage == 0: return stance(1)
    if stage == 1:
        return P(yaw=-24,root=-.014,pitch=13,lean=-5,head=3,
                 lu=[.18,-.34,-.92],lf=[.05,-.28,-.96],ru=[-.27,-.49,.83],rf=[-.08,-.20,.98],
                 lt=[.24,-.91,.33],rt=[-.20,-.95,.22])
    if stage == 2:
        return P(yaw=25,root=.016,pitch=24,lean=2,head=-5,
                 lu=[.12,-.10,.988],lf=[.03,.02,.999],ru=[-.24,-.48,.84],rf=[-.08,-.17,.982],
                 lt=[.20,-.43,.88],ls=[.08,-.62,.78],rt=[-.19,-.95,.23],rs=[-.08,-.99,-.03])
    return stance(2)


def light3_pose(stage):
    # Legacy frames 5-6: low replant -> forceful finishing fire arc/punch.
    if stage == 0:
        return P(yaw=-18,root=-.030,pitch=18,lean=-4,head=3,
                 ru=[-.22,-.55,-.80],rf=[-.07,-.37,-.93],lu=[.24,-.56,.79],lf=[.08,-.28,.96],
                 lt=[.24,-.76,.61],ls=[.10,-.88,-.47],rt=[-.25,-.91,.32])
    if stage == 1:
        return P(yaw=30,root=.012,pitch=19,lean=5,head=-6,
                 ru=[-.10,.12,.988],rf=[-.03,.28,.96],lu=[.26,-.44,.86],lf=[.08,-.17,.982],
                 lt=[.21,-.92,.32],rt=[-.24,-.88,.40])
    if stage == 2:
        return P(yaw=20,root=.004,pitch=14,lean=2,head=-4,
                 ru=[-.06,-.04,.997],rf=[-.02,.03,.999],lu=[.29,-.55,.78],lf=[.09,-.25,.965],
                 lt=[.21,-.95,.22],rt=[-.24,-.91,.34])
    return stance(0)


def heavy_pose(stage):
    # Heavy sheet 1-4: flaming load -> crouched coil -> broad sweep -> low slam finish.
    if stage==0:
        return P(yaw=-18,root=-.020,pitch=12,lean=-4,head=3,
                 ru=[-.24,-.45,.86],rf=[-.08,-.14,.987],lu=[.22,-.52,.82],lf=[.07,-.25,.966],
                 lt=[.23,-.91,.34],rt=[-.24,-.92,.30])
    if stage==1:
        return P(yaw=-34,root=-.046,pitch=25,lean=-7,head=5,
                 ru=[-.18,-.52,-.83],rf=[-.06,-.35,-.94],lu=[.25,-.58,.77],lf=[.08,-.31,.947],
                 lt=[.27,-.72,.64],ls=[.11,-.82,-.56],rt=[-.27,-.80,.53],rs=[-.11,-.91,-.39])
    if stage==2:
        return P(yaw=58,root=.006,pitch=17,lean=8,head=-8,
                 ru=[-.06,-.06,.996],rf=[-.02,.08,.996],lu=[.26,-.43,.86],lf=[.08,-.16,.984],
                 lt=[.21,-.94,.25],rt=[-.27,-.89,.36])
    return P(yaw=26,root=-.052,pitch=30,lean=4,head=-5,
             ru=[-.12,-.72,.68],rf=[-.04,-.82,.57],lu=[.24,-.52,.82],lf=[.08,-.27,.96],
             lt=[.25,-.70,.67],ls=[.11,-.74,-.66],rt=[-.28,-.78,.56],rs=[-.12,-.86,-.49])


def launcher_pose(stage):
    # Rising Attack sheet 1-4: two compact setup hits then a committed flaming rise.
    if stage==0:
        return P(yaw=-10,root=-.020,pitch=13,head=1,
                 ru=[-.18,-.42,.89],rf=[-.06,-.12,.991],lu=[.23,-.48,.85],lf=[.07,-.19,.98],
                 lt=[.22,-.90,.36],rt=[-.23,-.92,.31])
    if stage==1:
        return P(yaw=14,root=-.010,pitch=15,head=-2,
                 lu=[.10,-.06,.993],lf=[.03,.03,.999],ru=[-.25,-.48,.84],rf=[-.08,-.18,.98],
                 lt=[.22,-.91,.34],rt=[-.23,-.93,.28])
    if stage==2:
        return P(yaw=-8,root=-.046,pitch=22,head=-2,
                 ru=[-.18,-.58,-.79],rf=[-.06,-.42,-.90],lu=[.21,-.49,.85],lf=[.07,-.20,.978],
                 lt=[.24,-.72,.65],ls=[.10,-.80,-.59],rt=[-.24,-.75,.61],rs=[-.10,-.84,-.53])
    return P(yaw=16,root=.030,pitch=28,head=-5,
             ru=[-.10,.72,.68],rf=[-.03,.89,.45],lu=[.20,-.34,.92],lf=[.06,-.09,.994],
             lt=[.18,-.83,.53],rt=[-.18,-.82,.54])


def guard_pose(perfect=False):
    return P(yaw=12,root=-.008,pitch=6,lu=[.25,-.30,.92],lf=[.08,.20,.98],ru=[-.25,-.34,.90],rf=[-.08,.15,.985],
             lt=[.20,-.95,.23],rt=[-.24,-.94,.24], head=-7 if perfect else -2)


def hurt_pose():
    return P(yaw=-20,root=-.012,pitch=-15,lean=-7,lu=[.28,-.64,-.72],lf=[.10,-.74,-.66],ru=[-.28,-.58,-.76],rf=[-.10,-.72,-.68],lt=[.18,-.91,.37],rt=[-.20,-.91,.36])


def flow_cancel_pose(stage):
    # Presentation-only snap for a successful Flow Cancel; never adds gameplay latency.
    if stage == 0:
        return P(yaw=-9,root=-.014,pitch=12,lean=-3,head=4,
                 lu=[.24,-.50,.83],lf=[.07,-.18,.981],ru=[-.24,-.52,.82],rf=[-.07,-.20,.978],
                 lt=[.23,-.90,.36],rt=[-.25,-.92,.30])
    if stage == 1:
        return P(yaw=18,root=.006,pitch=15,lean=5,head=-7,
                 lu=[.25,-.44,.86],lf=[.07,-.12,.990],ru=[-.18,-.32,.93],rf=[-.05,-.08,.995],
                 lt=[.20,-.94,.27],rt=[-.26,-.89,.37])
    return stance(1)


def pursuit_commit_pose(heavy=False):
    # Compressed chase commitment: head stays on the opponent before the strike.
    return P(yaw=8 if not heavy else -12,root=-.026,pitch=29 if not heavy else 25,head=-8 if not heavy else 6,
             lu=[.17,-.36,-.92],lf=[.05,-.49,-.87],
             ru=[-.17,-.30,.94] if not heavy else [-.18,-.48,-.86],
             rf=[-.05,-.38,.92] if not heavy else [-.06,-.31,-.95],
             lt=[.18,-.72,.67],ls=[.08,-.92,.38],rt=[-.20,-.80,-.56],rs=[-.09,-.96,-.26])


def heavy_recovery_pose():
    return P(yaw=18,root=-.018,pitch=16,lean=2,head=-4,
             ru=[-.18,-.53,.83],rf=[-.06,-.31,.95],lu=[.25,-.49,.84],lf=[.08,-.19,.978],
             lt=[.22,-.92,.31],rt=[-.25,-.92,.30])


def launcher_recovery_pose():
    return P(yaw=10,root=-.008,pitch=14,lean=2,head=-5,
             ru=[-.20,-.46,.86],rf=[-.06,-.22,.974],lu=[.24,-.48,.84],lf=[.07,-.18,.981],
             lt=[.21,-.93,.29],rt=[-.25,-.92,.30])


def power_pose(stage):
    if stage==0: return stance(0)
    if stage==1: return P(yaw=0,root=-.010,pitch=3,lu=[.34,-.18,.92],lf=[.12,.05,.99],ru=[-.34,-.18,.92],rf=[-.12,.05,.99],lt=[.19,-.96,.20],rt=[-.19,-.96,.20])
    return P(yaw=0,root=.009,pitch=-3,lu=[.38,.10,.92],lf=[.14,.22,.96],ru=[-.38,.10,.92],rf=[-.14,.22,.96],lt=[.19,-.96,.20],rt=[-.19,-.96,.20])


def beam_pose(stage):
    if stage==0: return stance(0)
    if stage==1: return P(yaw=10,root=-.014,pitch=9,lu=[.16,-.38,.91],lf=[.05,-.10,.99],ru=[-.24,-.34,.91],rf=[-.08,-.04,.996])
    if stage==2: return P(yaw=15,root=.012,pitch=13,lu=[.12,-.24,.96],lf=[.04,.04,.998],ru=[-.28,-.26,.92],rf=[-.08,.04,.996])
    return stance(1)


def fire_pose(stage):
    if stage==0: return P(yaw=-20,root=-.018,pitch=15,ru=[-.22,-.44,-.87],rf=[-.07,-.32,-.94],lu=[.18,-.50,.85],lf=[.05,-.22,.97])
    if stage==1: return P(yaw=22,root=.005,pitch=17,ru=[-.16,-.10,.985],rf=[-.05,.02,.998],lu=[.14,-.48,.86],lf=[.04,-.20,.98])
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

    add("fighting_stance",.420,True,[stance(i) for i in range(4)],"Omega 3D translation of Legacy fightingStance stance_01..04 @105ms")
    add("combat_ready",.300,False,[combat_ready_pose(i) for i in range(3)],"Omega combat-entry transition into the Legacy fighting stance")
    add("combat_relax",.280,False,[combat_relax_pose(i) for i in range(3)],"Omega combat-exit transition back toward hub posture")
    add("run_start",.165,False,[run_start_pose(i) for i in range(3)],"Omega Story locomotion launch from a real Legacy idle boundary into the sprint")
    add("run",.300,True,[run_pose(i) for i in range(4)],"Update 2 four-key sprint keyed to Legacy RUN 1-4 silhouettes; 3D in-betweens preserve anatomy")
    add("run_stop",.180,False,[run_stop_pose(i) for i in range(3)],"Omega Story locomotion brake from sprint back to a real Legacy idle boundary")
    add("combat_advance",.320,True,[combat_advance_pose(i) for i in range(4)],"Omega guarded forward combat locomotion; opponent remains faced")
    add("combat_retreat",.340,True,[combat_retreat_pose(i) for i in range(4)],"Omega dedicated backpedal; replaces reverse-playing the hub run")
    add("dash",.240,False,[dash_pose(i) for i in range(4)],"Legacy dash_01..04 @60ms with stronger 3D launch silhouette")
    add("jump_start",.150,False,[jump_pose("start"),jump_pose("rise")],"Legacy jump_01..02 @75ms")
    add("fall",.220,True,[jump_pose("rise"),jump_pose("fall")],"Legacy jump_03..04 @110ms")
    add("land",.130,False,[jump_pose("land"),legacy_idle_pose(3)],"Omega Story landing: Legacy jump silhouette back to a Legacy idle boundary")
    add("combat_land",.130,False,[jump_pose("land"),stance(0)],"Omega combat landing: Legacy jump silhouette back to fighting stance")
    add("hard_land",.220,False,[hard_land_pose(i,False) for i in range(3)],"Omega Story heavy landing returning to Legacy idle boundary")
    add("combat_hard_land",.220,False,[hard_land_pose(i,True) for i in range(3)],"Omega combat heavy landing returning to fighting stance")
    add("light_1",.270,False,[light1_pose(i) for i in range(4)],
        "U5 Legacy Light 1: sprite key poses aligned to gameplay impact/recovery",
        [0.0,.045,.105,.270])
    add("light_2",.290,False,[light2_pose(i) for i in range(4)],
        "U5 Legacy Light 2: cross-body load -> long stepping silhouette -> guard",
        [0.0,.050,.125,.290])
    add("light_3",.390,False,[light3_pose(i) for i in range(4)],
        "U5 Legacy Light 3: low replant -> fire finisher -> confident reset",
        [0.0,.070,.155,.390])
    add("heavy",.620,False,[heavy_pose(0),heavy_pose(1),heavy_pose(2),heavy_pose(3),heavy_recovery_pose()],
        "U5 Heavy: Legacy load/coil/sweep/slam with readable fast-striker recovery",
        [0.0,.145,.265,.385,.620])
    add("launcher",.550,False,[launcher_pose(0),launcher_pose(1),launcher_pose(2),launcher_pose(3),launcher_recovery_pose()],
        "U5 Launcher: setup hits -> flaming rise -> controlled recovery",
        [0.0,.090,.165,.255,.550])
    add("air_light",.360,False,[jump_pose("rise"),attack_pose("R",.99,vertical=.05),jump_pose("fall")],"Legacy air attack key silhouettes; gameplay duration .36s")
    add("air_heavy",.520,False,[jump_pose("rise"),heavy_pose(1),jump_pose("fall")],"Legacy air heavy key silhouettes; gameplay duration .52s")
    add("pursuit_light",.320,False,[pursuit_commit_pose(False),dash_pose(2),attack_pose("R",.99),jump_pose("fall")],
        "U5 Pursuit Light: committed lock/chase silhouette -> strike -> fall",
        [0.0,.045,.145,.320])
    add("pursuit_heavy",.460,False,[pursuit_commit_pose(True),dash_pose(2),heavy_pose(2),heavy_pose(3),jump_pose("fall")],
        "U5 Pursuit Heavy: compressed chase read -> committed finisher -> follow-through",
        [0.0,.055,.185,.300,.460])
    add("grab",.380,False,[stance(0),attack_pose("L",.92),stance(1)],"Legacy combat grab timing; shared authored pose translation")
    add("block",.240,True,[guard_pose(False),guard_pose(True)],"Legacy blockHold block_02 @120ms")
    add("perfect_block",.140,False,[guard_pose(False),guard_pose(True)],"Legacy perfectBlock block_03..04 @70ms")
    add("flow_cancel",.180,False,[flow_cancel_pose(i) for i in range(3)],
        "U5 presentation-only Flow Cancel snap; no added gameplay recovery or latency",
        [0.0,.060,.180])
    add("hurt",.210,False,[hurt_pose(),stance(2),stance(0)],"U5 Legacy hurt recoil -> deliberate combat recovery",
        [0.0,.145,.210])
    add("charge",.276,True,[power_pose(0),power_pose(1),power_pose(2)],"Legacy chargeEnergy ultimate_01..03 @92ms")
    add("counter",.216,False,[guard_pose(False),guard_pose(True),heavy_pose(1)],"Legacy counter block_01/perfect_01/heavy_02 @72ms")
    add("breaker",.150,False,[stance(0),heavy_pose(1)],"Legacy breaker stance_01/heavy_02 @75ms")
    add("fire_blast",.420,False,[fire_pose(0),fire_pose(1),fire_pose(2)],"Omega stronger 3D Fire Blast release preserving Legacy fire/projectile vocabulary")
    add("object_swap",.320,False,[beam_pose(i) for i in range(4)],"Legacy Object Swap beam_01..04 @60-80ms translated to .32s")
    add("lens_activate",.300,False,[power_pose(i) for i in range(3)],"Legacy Lens power_01..03 @80ms translated to .30s")
    payload={"version":2,"source":"U5: Legacy/approved Rrvvfo sprite-sheet silhouettes remain key-pose authority; timing aligns key silhouettes to active windows and signature recoveries without changing the 39-joint rig","clips":clips}
    a.output.parent.mkdir(parents=True,exist_ok=True); a.output.write_text(json.dumps(payload,indent=2)+"\n")
    print(f"Authored {len(clips)} Omega Chapter-1 Rrvvfo clips -> {a.output}")

if __name__=="__main__": main()

local sk_scene = require('sk_scene')
local sk_animation = require('sk_animation')

local armature = sk_animation.build_armature_for_animations(sk_scene.scene.animations)

sk_animation.export_animations('animation', armature, sk_scene.scene.animations, '_geo', '_anim')
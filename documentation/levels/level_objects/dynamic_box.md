# @dynamic_box

Dynamic level collision geometry. Useful for moving platforms.
Must be a child of an [animation bone](./anim.md).

## Name structure

```
@dynamic_box
```

## Notes

Dynamic box colliders are animated when their parent bone is.

If a dynamic box moves into a portal which is not animated by its own animation,
the portal is closed.

If portalable [static geometry](./static.md) exists as a sibling (i.e., animated
by the same bone), the collider will be used for portal gun projectile hits and
take priority over any coplanar [static collision](./collision.md).

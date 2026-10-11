## Environment variables

Unless specified otherwise, a variable is enabled if and only if it's set to `1`

### DRM

`ABYSSAL_DRM_DEVICES` -> Set an explicit list of DRM devices (GPUs) to use. It's a colon-separated list of paths, with the first being the primary. E.g. `/dev/dri/card1:/dev/dri/card0`
`ABYSSAL_NO_ATOMIC` -> Disables drm atomic modesetting
`ABYSSAL_MGPU_NO_EXPLICIT` -> Disables explicit syncing on mgpu buffers
`ABYSSAL_NO_MODIFIERS` -> Disables modifiers for DRM buffers

### Input

`ABYSSAL_LIBINPUT_NO_PLUGINS` -> Disables libinput plugin loading

### Debugging

`ABYSSAL_TRACE` -> Enables trace (very verbose) logging

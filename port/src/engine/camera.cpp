/*
 * Engine — Camera
 *
 * PC equivalents of the PS1 camera functions documented in
 * include/ps1/camera.h:
 *   Camera_LoadToGTE     (0x80187320)
 *   Camera_Manager       (0x8013b584)
 *   Camera_RecordFrame   (0x80187350)
 *   Room_SetupCameraSlots(0x8018b320)
 *
 * Will eventually drive HAL_GTE_LoadMatrix per frame and own the
 * 4-slot room camera array. Empty translation unit until the
 * RoomDescriptor parser is wired up.
 */

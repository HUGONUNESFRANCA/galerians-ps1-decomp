/*
 * Engine — Scheduler (coroutine state machine port)
 *
 * Will host the PC equivalent of the PS1 cooperative coroutine
 * scheduler (16 slots × 7 coroutines = 112 simultaneous). Reference:
 *   - docs/MemoryMap.md → "Coroutine Scheduler — Arquitetura Completa"
 *   - include/ps1/state_machine.h
 *   - include/ps1/coroutine.h
 *
 * Empty translation unit until the slot dispatcher is ported.
 */

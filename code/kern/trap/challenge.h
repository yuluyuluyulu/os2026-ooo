#ifndef LAB3_CHALLENGE_H
#define LAB3_CHALLENGE_H
#include <trap.h>
void lab3_switch_test(void);
bool challenge_exception(struct trapframe *tf);
void challenge_timer_observe(struct trapframe *tf);
extern uintptr_t challenge_resume_sp;
#endif

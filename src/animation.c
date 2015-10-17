// @file animation.c
// @brief Animation framework to animate pointer values
//
// Animation framework to animate a pointer's value. Includes automatic
// detection of multiple animations per pointer, and destroys the oldest one.
// Animations also auto-destruct when complete
//
// @author Eric D. Phillips
// @date September 1, 2015
// @bugs No known bugs

#include "animation.h"
#include "utility.h"

// Animation constants
#define ANIMATION_TICK_INTERVALE 30     //< Number of milliseconds to pause between animation ticks

// Animation pointer type
typedef struct AnimationNode {
  void (*step_func)(struct AnimationNode*);   //< Function to call when stepping animation
  void                    *target;            //< Pointer to value being animated
  void                    *from;              //< Pointer to value to animate from
  void                    *to;                //< Pointer to value to animate to
  uint64_t                start_time;         //< Millisecond epoch of when animation was started
  uint32_t                duration;           //< Duration of animation in milliseconds
  struct AnimationNode    *next;              //< Pointer to next node in linked list
} AnimationNode;

// Animation framework data
static AnimationNode  *head_node = NULL;    //< Head node in linked list containing all animations
static AppTimer       *ani_timer = NULL;    //< AppTimer for stepping all animations

// Functions
static void prv_animation_timer_start(void);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Quadratic interpolation in out
static int32_t prv_curve_quad_ease_in_out(int32_t from, int32_t to, uint32_t percent,
                                          uint32_t percent_max) {
  if (percent >= percent_max) {
    return to;
  }
  int32_t t_percent = percent * 1000;
  t_percent /= percent_max / 2;
  if (t_percent < 1000) {
    return (to - from) / 2 * t_percent * t_percent / 1000000 + from;
  }
  t_percent -= 1000;
  return -(to - from) / 2 * (t_percent * (t_percent - 2000) - 1000000) / 1000000 + from;
}

// Step a int32 animation
static void prv_animation_step_int32(AnimationNode *node) {
  // step value
  int32_t from = (*(int32_t*)node->from);
  int32_t to = (*(int32_t*)node->to);
  uint32_t percent_max = node->duration;
  uint32_t percent = epoch() - node->start_time;
  (*(int32_t*)node->target) = prv_curve_quad_ease_in_out(from, to, percent, percent_max);
  // continue animation
  if (percent >= percent_max) {
    animation_stop(node->target);
  } else {
    prv_animation_timer_start();
  }
}

// Add node to end of linked list
static void prv_list_add_node(AnimationNode *node) {
  if (!head_node) {
    head_node = node;
    return;
  }
  AnimationNode *cur_node = head_node;
  while (cur_node->next) {
    cur_node = cur_node->next;
  }
  cur_node->next = node;
}

// Animation timer callback
static void prv_animation_timer_callback(void *data) {
  ani_timer = NULL;
  // loop over list and step each animation
  AnimationNode *cur_node = head_node;
  while (cur_node) {
    (*cur_node->step_func)(cur_node);
    cur_node = cur_node->next;
  }
  // TODO: Call animation tick callback
}

// Start animation timer if not running
static void prv_animation_timer_start(void) {
  if (!ani_timer) {
    ani_timer = app_timer_register(ANIMATION_TICK_INTERVALE, &prv_animation_timer_callback, NULL);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Functions
//

// Animate an integer by its pointer
void animation_int32_start(int32_t *ptr, int32_t to, uint32_t duration) {
  // create and add new node
  AnimationNode *new_node = (AnimationNode*)MALLOC(sizeof(AnimationNode));
  new_node->step_func = &prv_animation_step_int32;
  new_node->target = ptr;
  new_node->from = MALLOC(sizeof(int32_t));
  new_node->to = MALLOC(sizeof(int32_t));
  (*(int32_t*)new_node->from) = (*ptr);
  (*(int32_t*)new_node->to) = to;
  new_node->next = NULL;
  prv_list_add_node(new_node);
  // start animation timer if not running
  prv_animation_timer_start();
}

// Animate a GRect by its pointer
void animation_grect_start(GRect *ptr, GRect to, uint32_t duration) {
  // TODO: Develop this function
}

// Cancel an animation by its pointer
void animation_stop(void *ptr) {
  AnimationNode *cur_node = head_node;
  AnimationNode *pre_node = NULL;
  while (cur_node) {
    if (cur_node->target == ptr) {
      // link surrounding nodes
      if (pre_node) {
        pre_node->next = cur_node->next;
      } else {
        head_node = cur_node->next;
      }
      // destroy node
      free(cur_node->from);
      free(cur_node->to);
      free(cur_node);
      return;
    }
    pre_node = cur_node;
    cur_node = cur_node->next;
  }
}

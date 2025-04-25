/*
  ------------------------------------------------------------------------------
   Frugi
   Author: ydigikat
  ------------------------------------------------------------------------------
   MIT License
   Copyright (c) 2025 YDigiKat

   Permission to use, copy, modify, and/or distribute this code for any purpose
   with or without fee is hereby granted, provided the above copyright notice and
   this permission notice appear in all copies.
  ------------------------------------------------------------------------------
*/

#include "env_gen.h"


/* Ranges to convert normalised parameters back to useful values */
#define ENV_ATTACK_MS_MAX (10000.0f)
#define ENV_DECAY_MS_MAX (15000.0f)
#define ENV_RELEASE_MS_MAX (30000.0f)
#define ENV_ATTACK_MS_MIN (1.0f)
#define ENV_DECAY_MS_MIN (2.0f)
#define ENV_RELEASE_MS_MIN (2.0f)
#define ENV_POWER_EXP (1.5f)

/**  
 * recalc_coefficients
 * \brief Calculates the envelope segment coefficients for attack, decay and release curves
 * \note Implements Nigel Redmon's algorithm optimized for block processing rather than sample-by-sample.
 *       Operating at block rate provides computational efficiency:
 *       
 *       Update frequency = 375 Hz at 48kHz with 128-sample blocks
 * 
 *       The overshoot values ensure the asymptotic curves properly reach their targets.
 * 
 * \param env_gen Pointer to the envelope generator instance
 */
static void recalc_coefficients(struct env_gen *env_gen)
{
  float attack_blocks = env_gen->attack_param * env_gen->attack_scaler * env_gen->fsr / (1000.0f * env_gen->block_size);
  float decay_blocks = env_gen->decay_param * env_gen->decay_scaler * env_gen->fsr / (1000.0f * env_gen->block_size);
  float release_blocks = env_gen->release_param * env_gen->fsr / (1000.0f * env_gen->block_size);

  /* Compute base coefficients using block-based calculations */
  env_gen->attack_coeff = MATH_EXP(-logf((1.0f + env_gen->attack_tco) / env_gen->attack_tco) / attack_blocks);
  env_gen->attack_overshoot = (1.0f + env_gen->attack_tco) * (1.0f - env_gen->attack_coeff);

  env_gen->decay_coeff = MATH_EXP(-logf((1.0f + env_gen->decay_tco) / env_gen->decay_tco) / decay_blocks);
  env_gen->decay_overshoot = (env_gen->sustain_param - env_gen->decay_tco) * (1.0f - env_gen->decay_coeff);

  env_gen->release_coeff = MATH_EXP(-logf((1.0f + env_gen->release_tco) / env_gen->release_tco) / release_blocks);
  env_gen->release_overshoot = -env_gen->release_tco * (1.0f - env_gen->release_coeff);
}

/**  
 * env_state_off 
 * \brief Implements the OFF state - the final envelope state where output is zero
 * \param env_gen Pointer to the envelope generator instance
 */
static void env_state_off(struct env_gen *env_gen)
{
  env_gen->level = 0.0f;
}


/**  
 * env_state_attack
 * \brief Implements the ATTACK state - initial envelope segment with rising amplitude
 * \note Automatically transitions to DECAY state when:
 *       - Level reaches 1.0 (full amplitude)
 *       - Attack time is zero (immediate attack)
 * \param env_gen Pointer to the envelope generator instance
 */
static void env_state_attack(struct env_gen *env_gen)
{
  env_gen->level = env_gen->level * env_gen->attack_coeff + env_gen->attack_overshoot;

  if (env_gen->level >= 1.0f || env_gen->attack_param <= 0.0f)
  {
    env_gen->level = 1.0f;
    env_gen->state = ENV_DECAY;
  }
}

/**  
 * env_state_decay
 * \brief Implements the DECAY state - segment where amplitude falls to sustain level
 * \note Automatically transitions to SUSTAIN state when:
 *       - Level reaches the sustain level
 *       - Decay time is zero (immediate decay)
 * \param env_gen Pointer to the envelope generator instance
 */
static void env_state_decay(struct env_gen *env_gen)
{
  env_gen->level = env_gen->level * env_gen->decay_coeff + env_gen->decay_overshoot;

  if (env_gen->level <= env_gen->sustain_param || env_gen->decay_param <= 0.0f)
  {
    env_gen->level = env_gen->sustain_param;
    env_gen->state = ENV_SUSTAIN;
  }
}

/**  
 * env_state_sustain
 * \brief Implements the SUSTAIN state - maintains constant level until note-off
 * \note This state has no automatic transition - it remains active until explicitly
 *       changed by a note-off event triggering the RELEASE state.
 * \param env_gen Pointer to the envelope generator instance
 */
static void env_state_sustain(struct env_gen *env_gen)
{
  env_gen->level = env_gen->sustain_param;
}

/**
 * env_state_release
 * \brief Implements the RELEASE state - decreasing amplitude after note-off
 * \note Triggered by note-off event, not by an automatic state transition.
 *       Transitions to OFF state when amplitude reaches zero or release time is zero.
 * \param env_gen Pointer to the envelope generator instance 
 */
static void env_state_release(struct env_gen *env_gen)
{
  env_gen->level = env_gen->level * env_gen->release_coeff + env_gen->release_overshoot;

  if (env_gen->level <= 0.0f || env_gen->release_param <= 0.0f)
  {
    env_gen->level = 0.0f;
    env_gen->state = ENV_OFF;
  }
}

/**
 * env_state_shutdown
 * \brief Implements rapid amplitude fade to zero for voice termination
 * \note Used during voice stealing or RTZ (return to zero) operations when
 *       a voice needs to be quickly silenced. Uses linear ramp rather than
 *       exponential curve for predictable timing.
 * \param env_gen Pointer to the envelope generator instance 
 */
static void env_state_shutdown(struct env_gen *env_gen)
{

  env_gen->level += env_gen->inc_shutdown;

  /* Off */
  if (env_gen->level <= 0.0f)
  {
    env_gen->level = 0.0f; /* Clear any overshoot caused by RTZ */
    env_gen->state = ENV_OFF;
  }
}

/* The lookup (jump) table of state handlers */
static void (*env_state_handlers[ENV_MAX])(struct env_gen *env_gen) =
    {
        env_state_off,
        env_state_attack,
        env_state_decay,
        env_state_sustain,
        env_state_release,
        env_state_shutdown};


/* output transformer, this is used for a normal envelope */
static float env_mode_normal(float level, float sustain)
{
  return level;
}

/*  output transformer, this is used for a biased envelope (usually pitch)*/
static float env_mode_biased(float level, float sustain)
{
  return level - sustain;
}

/* output transformer, this is used for a inverted envelope */
static float env_mode_inverted(float level, float sustain)
{
  return 1.0f - level;
}

/*  output transformer, this is used for a biased and inverted envelope */
static float env_mode_biased_inverted(float level, float sustain)
{
  return (1.0f - level) - sustain;
}

/* The lookup (jump) table of output transformers  */
static float (*env_transforms[TRANSFORM_MAX])(float level, float sustain) =
    {
        env_mode_normal,
        env_mode_biased,
        env_mode_inverted,
        env_mode_biased_inverted};

void env_gen_init(struct env_gen *env_gen, float fsr, size_t block_size, float *env_level)
{
  RTT_ASSERT(env_gen != NULL);  

  env_gen->attack_tco = MATH_EXP(-1.5f);
  env_gen->decay_tco = MATH_EXP(-4.95f);
  env_gen->release_tco = env_gen->decay_tco;
  env_gen->fsr = fsr;
  env_gen->block_size = block_size;  
  env_gen->env_level = env_level;

  env_gen_reset(env_gen);

  RTT_LOG("EG initialised\n");
}

void env_gen_reset(struct env_gen *env_gen)
{
  RTT_ASSERT(env_gen != NULL);

  env_gen->state = ENV_OFF;
  env_gen->level = 0;
}

/**
 * env_gen_render
 * \brief Processes one block of the envelope generator state machine
 * \note Calls the appropriate state handler function based on current state
 *       and applies the configured output transform mode to the result.
 * \param env_gen Pointer to the envelope generator instance
 * \param block_size Number of samples to process (typically matches initialization value)
 */
void env_gen_render(struct env_gen *env_gen, size_t block_size)
{
  RTT_ASSERT(env_gen != NULL);  

  env_state_handlers[env_gen->state](env_gen);
  *env_gen->env_level = env_transforms[env_gen->mode_param](env_gen->level, env_gen->sustain_param);
}

/**
 * env_gen_note_on
 * \brief calc segment curves and start the envelope state machine 
 * \note the attack and decay are scaled by velocity/pitch if tracking is enabled, a higher
 *       velocity/note reduces the attack/decay time as you might expect from a piano.
 * \param env_gen the envelope instance 
 * \param midi_note the note for midi note scaling.
 * \param midi_velocity the midi velocity for scaling.
 */
void env_gen_note_on(struct env_gen *env_gen, uint8_t midi_note, uint8_t midi_velocity)
{
  RTT_ASSERT(env_gen != NULL);

  /* Store the scaling (tracking) values */
  env_gen->attack_scaler = env_gen->velocity_tracking_param ?  1.0f - midi_velocity / 127.0f : 1.0f;
  env_gen->decay_scaler = env_gen->note_tracking_param ? 1.0f - midi_note / 127.0f : 1.0f;

  recalc_coefficients(env_gen);

  /* Could use some smoothing here as we might be resetting an active envelope, but this is reliable */

  /* Move the env to the attack phase */
  env_gen->state = ENV_ATTACK;
}

/**
 * env_gen_note_off
 * \brief Triggers the release phase when a key is released
 * \note Transitions to RELEASE state if level is above zero,
 *       otherwise goes directly to OFF state.
 * \param env_gen Pointer to the envelope generator instance
 */
void env_gen_note_off(struct env_gen *env_gen)
{
  RTT_ASSERT(env_gen != NULL);

  if (env_gen->level > 0.0f)
  {
    env_gen->state = ENV_RELEASE;
  }
  else
  {
    env_gen->state = ENV_OFF;
  }
}

/**
 * env_gen_update_params
 * \brief Updates all envelope parameters and recalculates coefficients
 * \note All input parameters are in normalized range (0.0-1.0) and are
 *       internally converted to appropriate time values using power curves.
 * \param env_gen Pointer to the envelope generator instance
 * \param attack Attack time (0.0-1.0) - controls rise time
 * \param decay Decay time (0.0-1.0) - controls fall time to sustain
 * \param sustain Sustain level (0.0-1.0) - amplitude during held notes
 * \param release Release time (0.0-1.0) - controls fall time after note-off
 * \param mode Envelope transform mode (0.0-3.0) - controls output processing
 * \param note_tracking Enable/disable note-dependent decay (0.0=off, 1.0=on)
 * \param velocity_tracking Enable/disable velocity-dependent attack (0.0=off, 1.0=on)
 */
void env_gen_update_params(struct env_gen *env_gen, float attack, float decay, float sustain, float release, float mode, float note_tracking, float velocity_tracking)
{
  RTT_ASSERT(env_gen != NULL);

  env_gen->attack_param = PARAM_TO_POWER(attack, ENV_ATTACK_MS_MIN, ENV_ATTACK_MS_MAX, ENV_POWER_EXP);
  env_gen->decay_param = PARAM_TO_POWER(decay, ENV_DECAY_MS_MIN, ENV_DECAY_MS_MAX, ENV_POWER_EXP);
  env_gen->sustain_param = sustain;
  env_gen->release_param = PARAM_TO_POWER(release, ENV_RELEASE_MS_MIN, ENV_RELEASE_MS_MAX, ENV_POWER_EXP);
  env_gen->mode_param = mode;
  env_gen->note_tracking_param = note_tracking;
  env_gen->velocity_tracking_param = velocity_tracking;

  recalc_coefficients(env_gen);
}

/**
 * env_gen_rtz
 * \brief Return To Zero - rapidly silences the envelope
 * \note Used during voice stealing operations when a voice needs to be
 *       quickly repurposed for a new note. Creates a short linear ramp
 *       to zero regardless of current envelope state.
 * \param env_gen Pointer to the envelope generator instance
 */
void env_gen_rtz(struct env_gen *env_gen)
{
  RTT_ASSERT(env_gen != NULL);

  if (env_gen->level > 0.0f)
  {
    /* Calculate the number of blocks needed to fade to 0 */
    float shutdown_blocks = env_gen->block_size / env_gen->fsr; /* Time in seconds per block */
    float total_shutdown_time = fmaxf(shutdown_blocks, 1.0f);   /* Ensure at least one block */

    /* Compute the decrement step per block */
    env_gen->inc_shutdown = -(env_gen->level / total_shutdown_time);

    /* Move to shutdown state */
    env_gen->state = ENV_SHUTDOWN;
  }
}

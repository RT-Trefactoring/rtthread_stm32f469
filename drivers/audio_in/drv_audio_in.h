#ifndef __DRV_AUDIO_IN_H__
#define __DRV_AUDIO_IN_H__

#define PCM_DATA_LEN    (64)
extern rt_mailbox_t mic_start_record(int fifo_len);
extern int mic_stop_record(void);
extern void pcm_stereo_to_mono(const uint16_t * stereo_buffer, size_t stereo_len, uint16_t * mono_buffer);

#endif

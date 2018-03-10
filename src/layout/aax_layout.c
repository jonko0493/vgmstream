#include "layout.h"
#include "../vgmstream.h"
#include "../coding/coding.h"

aax_codec_data* init_layout_aax(int segment_count) {
    aax_codec_data *data = NULL;

    if (segment_count <= 0 || segment_count > 255)
        goto fail;

    data = calloc(1, sizeof(aax_codec_data));
    if (!data) goto fail;

    data->segment_count = segment_count;
    data->current_segment = 0;

    data->segments = calloc(segment_count, sizeof(VGMSTREAM*));
    if (!data->segments) goto fail;

    return data;
fail:
    free_layout_aax(data);
    return NULL;
}


void render_vgmstream_aax(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream) {
    int samples_written=0;
    aax_codec_data *data = vgmstream->layout_data;
    //int samples_per_frame = get_vgmstream_samples_per_frame(vgmstream);

    while (samples_written<sample_count) {
        int samples_to_do;
        int samples_this_block = data->segments[data->current_segment]->num_samples;

        if (vgmstream->loop_flag && vgmstream_do_loop(vgmstream)) {
            data->current_segment = data->loop_segment;

            reset_vgmstream(data->segments[data->current_segment]);

            vgmstream->samples_into_block = 0;
            continue;
        }

        samples_to_do = vgmstream_samples_to_do(samples_this_block, 1, vgmstream);

        if (samples_written+samples_to_do > sample_count)
            samples_to_do=sample_count-samples_written;

        if (samples_to_do == 0) {
            data->current_segment++;
            reset_vgmstream(data->segments[data->current_segment]);

            vgmstream->samples_into_block = 0;
            continue;
        }

        render_vgmstream(&buffer[samples_written*data->segments[data->current_segment]->channels],
                samples_to_do,data->segments[data->current_segment]);

        samples_written += samples_to_do;
        vgmstream->current_sample += samples_to_do;
        vgmstream->samples_into_block+=samples_to_do;
    }
}


void free_layout_aax(aax_codec_data *data) {
    int i;

    if (!data)
        return;

    if (data->segments) {
        for (i = 0; i < data->segment_count; i++) {
            /* note that the close_streamfile won't do anything but deallocate itself,
             * there is only one open file in vgmstream->ch[0].streamfile */
            close_vgmstream(data->segments[i]);
        }
        free(data->segments);
    }
    free(data);
}

void reset_layout_aax(aax_codec_data *data) {
    int i;

    if (!data)
        return;

    data->current_segment = 0;
    for (i = 0; i < data->segment_count; i++) {
        reset_vgmstream(data->segments[i]);
    }
}

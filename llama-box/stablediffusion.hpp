#pragma once

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stable-diffusion.cpp/thirdparty/stb_image.h"
#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stable-diffusion.cpp/thirdparty/stb_image_resize.h"
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stable-diffusion.cpp/thirdparty/stb_image_write.h"

#include "stable-diffusion.cpp/model.h"
#include "stable-diffusion.cpp/stable-diffusion.h"

struct stablediffusion_params_sampling {
    uint32_t seed                    = LLAMA_DEFAULT_SEED;
    int height                       = 1024;
    int width                        = 1024;
    float guidance                   = 3.5f;
    float strength                   = 0.0f;
    sample_method_t sample_method    = N_SAMPLE_METHODS;
    int sampling_steps               = 0;
    float cfg_scale                  = 0.0f;
    float slg_scale                  = 0.0f;
    std::vector<int> slg_skip_layers = {7, 8, 9};
    float slg_start                  = 0.01;
    float slg_end                    = 0.2;
    schedule_t schedule_method       = DISCRETE;
    std::string negative_prompt;
    float control_strength      = 0.9f;
    bool control_canny          = false;
    uint8_t *control_img_buffer = nullptr;
    uint8_t *init_img_buffer    = nullptr;
    uint8_t *mask_img_buffer    = nullptr;
};

struct stablediffusion_params {
    int max_batch_count = 4;
    stablediffusion_params_sampling sampling;
    bool text_encoder_model_offload = true;
    std::string clip_l_model;
    std::string clip_g_model;
    std::string t5xxl_model;
    bool vae_model_offload = true;
    std::string vae_model;
    bool vae_tiling = false;
    std::string taesd_model;
    std::string upscale_model;
    int upscale_repeats        = 1;
    bool control_model_offload = true;
    std::string control_net_model;
    bool free_compute_immediately = false;

    // inherited from common_params
    std::string model;
    std::string model_alias;
    uint32_t seed                                       = LLAMA_DEFAULT_SEED;
    bool warmup                                         = true;
    bool flash_attn                                     = false;
    int n_threads                                       = 1;
    bool lora_init_without_apply                        = false;
    std::vector<common_adapter_lora_info> lora_adapters = {};
    float *tensor_split                                 = nullptr;
};

struct stablediffusion_sampling_stream {
    sd_sampling_stream_t *stream;
};

struct stablediffusion_generated_image {
    int size;
    unsigned char *data;
};

class stablediffusion_context {
  public:
    stablediffusion_context(sd_ctx_t *sd_ctx, upscaler_ctx_t *upscaler_ctx, stablediffusion_params params)
        : sd_ctx(sd_ctx), upscaler_ctx(upscaler_ctx), params(params) {
    }

    ~stablediffusion_context();

    float get_default_strength();
    sample_method_t get_default_sample_method();
    int get_default_sampling_steps();
    float get_default_cfg_scale();
    std::pair<int, int> get_default_image_size();
    void apply_lora_adapters(std::vector<common_adapter_lora_info> &lora_adapters);
    stablediffusion_sampling_stream *generate_stream(const char *prompt, stablediffusion_params_sampling sparams);
    bool sample_stream(stablediffusion_sampling_stream *stream);
    std::pair<int, int> progress_stream(stablediffusion_sampling_stream *stream);
    stablediffusion_generated_image preview_image_stream(stablediffusion_sampling_stream *stream, bool faster = false);
    stablediffusion_generated_image result_image_stream(stablediffusion_sampling_stream *stream);

  private:
    sd_ctx_t *sd_ctx             = nullptr;
    upscaler_ctx_t *upscaler_ctx = nullptr;
    stablediffusion_params params;
};

stablediffusion_context::~stablediffusion_context() {
    if (sd_ctx != nullptr) {
        free_sd_ctx(sd_ctx);
        sd_ctx = nullptr;
    }
    if (upscaler_ctx != nullptr) {
        free_upscaler_ctx(upscaler_ctx);
        upscaler_ctx = nullptr;
    }
}

float stablediffusion_context::get_default_strength() {
    switch (sd_get_version(sd_ctx)) {
        case VERSION_SD1_INPAINT:
        case VERSION_SD2_INPAINT:
        case VERSION_SDXL_INPAINT:
        case VERSION_FLUX_FILL:
            return 1.0f;
        default:
            return 0.75f;
    }
}

sample_method_t stablediffusion_context::get_default_sample_method() {
    switch (sd_get_version(sd_ctx)) {
        case VERSION_SD1_INPAINT:
            return EULER_A;
        case VERSION_SD2_INPAINT:
        case VERSION_SDXL_INPAINT:
        case VERSION_FLUX_FILL:
            return EULER;

        case VERSION_SD1:
        case VERSION_SD2: // including Turbo
            return EULER_A;
        case VERSION_SDXL: // including Turbo
        case VERSION_SDXL_REFINER:
        case VERSION_SD3:  // including Turbo
        case VERSION_FLUX: // including Schnell
            return EULER;
        default:
            return EULER_A;
    }
}

int stablediffusion_context::get_default_sampling_steps() {
    switch (sd_get_version(sd_ctx)) {
        case VERSION_SD1_INPAINT:
        case VERSION_SD2_INPAINT:
        case VERSION_SDXL_INPAINT:
        case VERSION_FLUX_FILL:
            return 50;

        case VERSION_SD1:
        case VERSION_SD2: // including Turbo
            return 20;
        case VERSION_SDXL: // including Turbo
        case VERSION_SDXL_REFINER:
            return 25;
        case VERSION_SD3:  // including Turbo
        case VERSION_FLUX: // including Schnell
        default:
            return 20;
    }
}

float stablediffusion_context::get_default_cfg_scale() {
    switch (sd_get_version(sd_ctx)) {
        case VERSION_SD1_INPAINT:
        case VERSION_SD2_INPAINT:
            return 9.0f;
        case VERSION_SDXL_INPAINT:
            return 5.0f;
        case VERSION_FLUX_FILL:
            return 3.5f;

        case VERSION_SD1:
        case VERSION_SD2: // including Turbo
            return 9.0f;
        case VERSION_SDXL: // including Turbo
        case VERSION_SDXL_REFINER:
            return 5.0f;
        case VERSION_SD3: // including Turbo
            return 4.5f;
        case VERSION_FLUX: // including Schnell
            return 1.0f;
        default:
            return 4.5f;
    }
}

std::pair<int, int> stablediffusion_context::get_default_image_size() {
    // { height, width }
    switch (sd_get_version(sd_ctx)) {
        case VERSION_SD1_INPAINT:
        case VERSION_SD2_INPAINT:
        case VERSION_SDXL_INPAINT:
            return {512, 512};
        case VERSION_FLUX_FILL:
            return {1024, 1024};

        case VERSION_SD1:
        case VERSION_SD2: // including Turbo
            return {512, 512};
        case VERSION_SDXL: // including Turbo
        case VERSION_SDXL_REFINER:
        case VERSION_SD3:  // including Turbo
        case VERSION_FLUX: // including Schnell
        default:
            return {1024, 1024};
    }
}

void stablediffusion_context::apply_lora_adapters(std::vector<common_adapter_lora_info> &lora_adapters) {
    std::vector<sd_lora_adapter_container_t> sd_lora_adapters;
    for (auto &lora_adapter : lora_adapters) {
        sd_lora_adapters.push_back({lora_adapter.path.c_str(), lora_adapter.scale});
    }
    sd_lora_adapters_apply(sd_ctx, sd_lora_adapters);
}

stablediffusion_sampling_stream *stablediffusion_context::generate_stream(const char *prompt, stablediffusion_params_sampling sparams) {
    int clip_skip = -1;
    int64_t seed  = sparams.seed;
    if (seed == LLAMA_DEFAULT_SEED) {
        seed = -1;
    }

    sd_sampling_stream_t *stream = nullptr;
    if (sparams.init_img_buffer != nullptr) {
        auto init_img           = sd_image_t{uint32_t(sparams.width), uint32_t(sparams.height), 3, sparams.init_img_buffer};
        auto mask_img           = sd_image_t{uint32_t(sparams.width), uint32_t(sparams.height), 1, sparams.mask_img_buffer};
        sd_image_t *control_img = nullptr;
        if (sparams.control_img_buffer != nullptr) {
            control_img = new sd_image_t{uint32_t(sparams.width), uint32_t(sparams.height), 3, sparams.control_img_buffer};
        }
        stream = img2img_stream(
            sd_ctx,
            init_img,
            mask_img,
            prompt,
            sparams.negative_prompt.c_str(),
            clip_skip,
            sparams.cfg_scale,
            sparams.guidance,
            sparams.width,
            sparams.height,
            sparams.sample_method,
            sparams.schedule_method,
            sparams.sampling_steps,
            sparams.strength,
            seed,
            control_img,
            sparams.control_strength,
            sparams.slg_skip_layers.data(),
            sparams.slg_skip_layers.size(),
            sparams.slg_scale,
            sparams.slg_start,
            sparams.slg_end);
    } else {
        stream = txt2img_stream(
            sd_ctx,
            prompt,
            sparams.negative_prompt.c_str(),
            clip_skip,
            sparams.cfg_scale,
            sparams.guidance,
            sparams.width,
            sparams.height,
            sparams.sample_method,
            sparams.schedule_method,
            sparams.sampling_steps,
            seed,
            nullptr,
            sparams.control_strength,
            sparams.slg_skip_layers.data(),
            sparams.slg_skip_layers.size(),
            sparams.slg_scale,
            sparams.slg_start,
            sparams.slg_end);
    }

    return new stablediffusion_sampling_stream{
        stream,
    };
}

bool stablediffusion_context::sample_stream(stablediffusion_sampling_stream *stream) {
    if (stream == nullptr) {
        return false;
    }

    return sd_sampling_stream_sample(sd_ctx, stream->stream);
}

std::pair<int, int> stablediffusion_context::progress_stream(stablediffusion_sampling_stream *stream) {
    if (stream == nullptr) {
        return {0, 1};
    }

    return {sd_sampling_stream_sampled_steps(stream->stream), sd_sampling_stream_steps(stream->stream)};
}

stablediffusion_generated_image stablediffusion_context::preview_image_stream(stablediffusion_sampling_stream *stream, bool faster) {
    if (stream == nullptr) {
        return stablediffusion_generated_image{0, nullptr};
    }

    sd_image_t img = sd_sampling_stream_get_preview_image(sd_ctx, stream->stream, faster);
    if (img.data == nullptr) {
        return stablediffusion_generated_image{0, nullptr};
    }

    int size            = 0;
    unsigned char *data = stbi_write_png_to_mem(
        (stbi_uc *)img.data,
        0,
        (int)img.width,
        (int)img.height,
        (int)img.channel,
        &size,
        nullptr);
    if (data == nullptr || size <= 0) {
        return stablediffusion_generated_image{0, nullptr};
    }

    return stablediffusion_generated_image{size, data};
}

stablediffusion_generated_image stablediffusion_context::result_image_stream(stablediffusion_sampling_stream *stream) {
    if (stream == nullptr) {
        return stablediffusion_generated_image{0, nullptr};
    }

    sd_image_t img = sd_sampling_stream_get_image(sd_ctx, stream->stream);
    if (img.data == nullptr) {
        return stablediffusion_generated_image{0, nullptr};
    }

    int upscale_factor = 4;
    if (upscaler_ctx != nullptr && params.upscale_repeats > 0) {
        for (int u = 0; u < params.upscale_repeats; ++u) {
            sd_image_t upscaled_img = upscale(upscaler_ctx, img, upscale_factor);
            if (upscaled_img.data == nullptr) {
                LOG_WRN("%s: failed to upscale image\n", __func__);
                break;
            }
            stbi_image_free(img.data);
            img = upscaled_img;
        }
    }

    int size            = 0;
    const char *param   = sd_sampling_stream_get_parameters_str(stream->stream);
    unsigned char *data = stbi_write_png_to_mem(
        (stbi_uc *)img.data,
        0,
        (int)img.width,
        (int)img.height,
        (int)img.channel,
        &size,
        param);
    if (data == nullptr || size <= 0) {
        return stablediffusion_generated_image{0, nullptr};
    }

    return stablediffusion_generated_image{size, data};
}

stablediffusion_context *common_sd_init_from_params(stablediffusion_params params) {
    std::string diffusion_model;
    std::string embed_dir;
    std::string stacked_id_embed_dir;
    std::string lora_model_dir;
    auto wtype                   = sd_type_t(GGML_TYPE_COUNT);
    rng_type_t rng_type          = CUDA_RNG;
    bool vae_decode_only         = false;
    bool free_params_immediately = false;
    bool tae_preview_only        = false;

    sd_ctx_t *sd_ctx = new_sd_ctx(
        params.model.c_str(),
        params.clip_l_model.c_str(),
        params.clip_g_model.c_str(),
        params.t5xxl_model.c_str(),
        diffusion_model.c_str(),
        params.vae_model.c_str(),
        params.taesd_model.c_str(),
        params.control_net_model.c_str(),
        lora_model_dir.c_str(),
        embed_dir.c_str(),
        stacked_id_embed_dir.c_str(),
        vae_decode_only,
        params.vae_tiling,
        free_params_immediately,
        params.free_compute_immediately,
        params.n_threads,
        wtype,
        rng_type,
        params.sampling.schedule_method,
        !params.text_encoder_model_offload,
        !params.control_model_offload,
        !params.vae_model_offload,
        params.flash_attn,
        tae_preview_only,
        params.tensor_split);
    if (sd_ctx == nullptr) {
        LOG_ERR("%s: failed to create stable diffusion context\n", __func__);
        return nullptr;
    }

    upscaler_ctx_t *upscaler_ctx = nullptr;
    if (!params.upscale_model.empty()) {
        upscaler_ctx = new_upscaler_ctx(
            params.upscale_model.c_str(),
            params.n_threads,
            params.tensor_split);
        if (upscaler_ctx == nullptr) {
            LOG_ERR("%s: failed to create upscaler context\n", __func__);
            free_sd_ctx(sd_ctx);
            return nullptr;
        }
    }

    if (!params.lora_init_without_apply && !params.lora_adapters.empty()) {
        std::vector<sd_lora_adapter_container_t> lora_adapters;
        for (auto &la : params.lora_adapters) {
            lora_adapters.push_back({la.path.c_str(), la.scale});
        }
        sd_lora_adapters_apply(sd_ctx, lora_adapters);
    }

    auto sc = new stablediffusion_context(sd_ctx, upscaler_ctx, params);
    if (params.warmup) {
        LOG_WRN("%s: warming up the model with an empty run - please wait ... (--no-warmup to disable)\n", __func__);

        stablediffusion_params_sampling wparams = params.sampling;
        wparams.sampling_steps                  = 1; // sample only once
        wparams.sample_method                   = EULER;
        wparams.schedule_method                 = DISCRETE;
        stablediffusion_sampling_stream *stream = sc->generate_stream("a lovely cat", wparams);
        sc->sample_stream(stream);
        stablediffusion_generated_image img = sc->result_image_stream(stream);
        if (img.data != nullptr) {
            stbi_image_free(img.data);
        }
        sd_sampling_stream_free(stream->stream);
        delete stream;
    }

    return sc;
}

static void sd_log_set(sd_log_cb_t cb, void *data) {
    sd_set_log_callback(cb, data);
}

static void sd_progress_set(sd_progress_cb_t cb, void *data) {
    sd_set_progress_callback(cb, data);
}

static ggml_log_level sd_log_level_to_ggml_log_level(sd_log_level_t level) {
    switch (level) {
        case SD_LOG_INFO:
            return GGML_LOG_LEVEL_INFO;
        case SD_LOG_WARN:
            return GGML_LOG_LEVEL_WARN;
        case SD_LOG_ERROR:
            return GGML_LOG_LEVEL_ERROR;
        default:
            return GGML_LOG_LEVEL_DEBUG;
    }
}
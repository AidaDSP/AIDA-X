/*
 * AIDA-X DPF plugin
 * Copyright (C) 2022-2023 Massimo Pennazio <maxipenna@libero.it>
 * Copyright (C) 2023-2024 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "DistrhoUtils.hpp"

#include "model_variant.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

void modelActivate(ModelVariantType& model)
{
    std::visit (
        [] (auto&& custom_model)
        {
            using ModelType = std::decay_t<decltype (custom_model)>;
            if constexpr (! std::is_same_v<ModelType, NullModel>)
            {
                custom_model.reset();
            }
        },
        model);
}

void modelLoad(ModelVariantType& model, nlohmann::json& model_json)
{
    std::visit (
        [&model_json] (auto&& custom_model)
        {
            using ModelType = std::decay_t<decltype (custom_model)>;
            if constexpr (! std::is_same_v<ModelType, NullModel>)
            {
                custom_model.parseJson (model_json, true);
                custom_model.reset();
            }
        },
        model);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

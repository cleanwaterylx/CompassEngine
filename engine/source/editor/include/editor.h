#pragma once

#include "runtime/core/math/vector2.h"

#include <memory>

namespace Compass
{
    class EditorUI;
    class CompassEngine;

    class CompassEditor
    {
        friend class EditorUI;

    public:
        CompassEditor();
        virtual ~CompassEditor();

        void initialize(CompassEngine* engine_runtime);
        void clear();

        void run();

    protected:
        std::shared_ptr<EditorUI> m_editor_ui;
        CompassEngine* m_engine_runtime { nullptr };
    };
} // namespace Compass

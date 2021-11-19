#pragma once

#include "common/PdfApplication.h"

class DebugApplication : public PdfApplication {
  public:
    static Glib::RefPtr<DebugApplication> create() {
        return Glib::make_refptr_for_instance<DebugApplication>(new DebugApplication());
    }

    void open_window(const std::string &filePath) override;

  protected:
    DebugApplication();
};

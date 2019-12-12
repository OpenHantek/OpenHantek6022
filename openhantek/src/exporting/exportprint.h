// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "exporterinterface.h"

class ExporterPrint : public ExporterInterface
{
public:
    ExporterPrint();
    void create(ExporterRegistry *registry) override;
    int faIcon() override;
    QString name() override;
    Type type() override;
    bool samples(const std::shared_ptr<PPresult>data) override;
    bool save() override;
    float progress() override;
private:
    std::shared_ptr<PPresult> data;
};

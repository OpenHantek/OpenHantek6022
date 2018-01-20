// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "exporterinterface.h"

class ExporterImage : public ExporterInterface
{
public:
    ExporterImage();
    virtual void create(ExporterRegistry *registry) override;
    virtual QIcon icon() override;
    virtual QString name() override;
    virtual Type type() override;
    virtual bool samples(const std::shared_ptr<PPresult>data) override;
    virtual bool save() override;
    virtual float progress() override;
private:
    std::shared_ptr<PPresult> data;
};

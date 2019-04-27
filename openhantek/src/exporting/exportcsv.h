// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "exporterinterface.h"

class ExporterCSV : public ExporterInterface
{
public:
    ExporterCSV();
    void create(ExporterRegistry *registry) override;
    QIcon icon() override;
    QString name() override;
    Type type() override;
    bool samples(const std::shared_ptr<PPresult>data) override;
    bool save() override;
    float progress() override;
private:
    std::shared_ptr<PPresult> data;
};

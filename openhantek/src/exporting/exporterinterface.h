// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QCoreApplication>
#include <QIcon>
#include <QString>

#include <memory>

class ExporterRegistry;
class PPresult;

/**
 * Implement this interface and register your Exporter to the ExporterRegistry instance
 * in the main routine to make an Exporter available.
 */
class ExporterInterface {
  public:
    virtual ~ExporterInterface();
    /**
     * Starts up this exporter. Acquires resources etc. Do not call this directly, it
     * will be called by the exporter registry at some point. Release your resources in the
     * destructor as usual.
     * @param registry The exporter registry instance. This is used to obtain a reference
     *        to the settings.
     */
    virtual void create( ExporterRegistry *registry ) = 0;

    /**
     * @return Return the icon representation of this exporter. Will be used in graphical
     * interfaces.
     */
    virtual int faIcon() = 0;

    /**
     * @return Return the text representation / name of this exporter. Will be used in graphical
     * interfaces.
     */
    virtual QString name() = 0;

    /**
     * @return Return this exporter's data format. Will be used in graphical interfaces.
     */
    virtual QString format() = 0;

    /**
     * Exporters can save only a single sample set or save data continuously.
     */
    enum class Type { SnapshotExport, ContinuousExport };

    /**
     * @return Return the type of this exporter.
     * @see ExporterInterface::Type
     */
    virtual Type type() = 0;

    /**
     * A new sample set from the ExporterRegistry. The exporter needs to be active to receive samples.
     * If it is a snapshot exporter, only one set of samples will be received.
     * @return Return true if you want to receive another sample or false if you are done (progress()==1).
     */
    virtual bool samples( const std::shared_ptr< PPresult > ) = 0;

    /**
     * Exporter: Save your received data and perform any conversions necessary.
     * This method will be called in the
     * GUI thread context and can create and show dialogs if required.
     * @return Return true if saving succeeded otherwise false.
     */
    virtual bool save() = 0;

    /**
     * @brief The progress of receiving and processing samples. If the exporter returns 1, it will
     * be called back by the GUI via the save() method.
     *
     * @return A number between 0..1 indicating the used capacity of this exporter. If this is a
     * snapshot exporter, only 0 for "no samples processed yet" or 1 for "finished" will be returned.
     * A continuous exporter may report the used memory / reserved memory ratio here.
     */
    virtual float progress() = 0;

  protected:
    ExporterRegistry *registry;
};

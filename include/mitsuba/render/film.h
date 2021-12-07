#pragma once

#include <mitsuba/mitsuba.h>
#include <mitsuba/core/logger.h>
#include <mitsuba/core/object.h>
#include <mitsuba/core/rfilter.h>
#include <mitsuba/core/vector.h>
#include <mitsuba/render/sampler.h>
#include <mitsuba/render/fwd.h>
#include <mitsuba/render/texture.h>

NAMESPACE_BEGIN(mitsuba)


/**
 * \brief This list of flags is used to classify the different types of films.
 */
enum class FilmFlags : uint32_t {
    // =============================================================
    //                      Emitter types
    // =============================================================

    /// No flags set (default value)
    None                 = 0x00000,

    /// The film stores a spectral representation of the image
    Spectral             = 0x00001,

    /**
     * The film provides a customized \ref prepare_sample() routine that implements
     * a special treatment of the samples before storing them in the Image Block.
     */
    Special              = 0x00002,
};

MTS_DECLARE_ENUM_OPERATORS(FilmFlags)


/** \brief Abstract film base class - used to store samples
 * generated by \ref Integrator implementations.
 *
 * To avoid lock-related bottlenecks when rendering with many cores,
 * rendering threads first store results in an "image block", which
 * is then committed to the film using the \ref put() method.
 */
template <typename Float, typename Spectrum>
class MTS_EXPORT_RENDER Film : public Object {
public:
    MTS_IMPORT_TYPES(ImageBlock, ReconstructionFilter, Texture)

    /**
     * Configure the film for rendering a specified set of extra channels (AOVS).
     * Returns the total number of channels that the film will store
     */
    virtual size_t prepare(const std::vector<std::string> &aovs) = 0;

    /// Merge an image block into the film. This methods should be thread-safe.
    virtual void put(const ImageBlock *block) = 0;

    /// Return a image buffer object storing the developed image
    virtual TensorXf develop(bool raw = false) const = 0;

    /// Return a bitmap object storing the developed contents of the film
    virtual ref<Bitmap> bitmap(bool raw = false) const = 0;

    /// Write the developed contents of the film to a file on disk
    virtual void write(const fs::path &path) const = 0;

    /// ek::schedule() variables that represent the internal film storage
    virtual void schedule_storage() = 0;

    /// Prepare spectrum samples to be in the format expected by the film
    virtual void prepare_sample(const UnpolarizedSpectrum &spec,
                                const Wavelength &wavelengths,
                                Float* aovs, Mask active) const;

    /**
     * \brief Return a reference to a newly created storage similar to the
     * underlying one used by the film
     *
     * \param normalize
     *    Enable normalization in the film's storage
     * \param borders
     *    Enable quality borders in the film's storage independently of
     *    film's configuration
     */
    virtual ref<ImageBlock> create_storage(bool normalize=false, bool borders=false) = 0;

    // =============================================================
    //! @{ \name Accessor functions
    // =============================================================

    /**
     * Should regions slightly outside the image plane be sampled to improve
     * the quality of the reconstruction at the edges? This only makes
     * sense when reconstruction filters other than the box filter are used.
     */
    bool has_high_quality_edges() const { return m_high_quality_edges; }

    /// Ignoring the crop window, return the resolution of the underlying sensor
    const ScalarVector2u &size() const { return m_size; }

    /// Return the size of the crop window
    const ScalarVector2u &crop_size() const { return m_crop_size; }

    /// Return the offset of the crop window
    const ScalarPoint2u &crop_offset() const { return m_crop_offset; }

    /// Set the size and offset of the crop window.
    void set_crop_window(const ScalarPoint2u &crop_offset,
                         const ScalarVector2u &crop_size);

    /// Return the image reconstruction filter (const version)
    const ReconstructionFilter *reconstruction_filter() const {
        return m_filter.get();
    }

    /// Returns the specific Sensor Response Function (SRF) used by the film
    const Texture *sensor_response_function();

    /// Flags for all properties combined.
    uint32_t flags() const { return m_flags; }

    //! @}
    // =============================================================

    virtual std::string to_string() const override;

    MTS_DECLARE_CLASS()
protected:
    /// Create a film
    Film(const Properties &props);

    /// Virtual destructor
    virtual ~Film();

    /// Combined flags for all properties of this film.
    uint32_t m_flags;

protected:
    ScalarVector2u m_size;
    ScalarVector2u m_crop_size;
    ScalarPoint2u m_crop_offset;
    bool m_high_quality_edges;
    ref<ReconstructionFilter> m_filter;
    ref<Texture> m_srf;
};

MTS_EXTERN_CLASS_RENDER(Film)
NAMESPACE_END(mitsuba)

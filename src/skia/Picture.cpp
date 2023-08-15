#include "common.h"
#include <include/core/SkDrawable.h>
#include <include/core/SkBBHFactory.h>
#include <include/core/SkPictureRecorder.h>
#include <pybind11/operators.h>

namespace {

class PyPicture : public SkPicture {
public:
    void playback(
        SkCanvas *canvas, SkPicture::AbortCallback *callback=nullptr
        ) const override {
        PYBIND11_OVERRIDE_PURE(void, SkPicture, playback, canvas, callback);
    }
    SkRect cullRect() const override {
        PYBIND11_OVERRIDE_PURE(SkRect, SkPicture, cullRect);
    }
    int approximateOpCount(bool nested=false) const override {
        PYBIND11_OVERRIDE_PURE(int, SkPicture, approximateOpCount, nested);
    }
    size_t approximateBytesUsed() const override {
        PYBIND11_OVERRIDE_PURE(size_t, SkPicture, approximateBytesUsed);
    }
};

class PyBBoxHierarchy : public SkBBoxHierarchy {
public:
    using SkBBoxHierarchy::SkBBoxHierarchy;
    void insert(const SkRect rects[], int N) override {
        PYBIND11_OVERRIDE_PURE(void, SkBBoxHierarchy, insert, rects, N);
    }
    void search(const SkRect& query, std::vector<int> *results) const override {
        PYBIND11_OVERRIDE_PURE(void, SkBBoxHierarchy, search, query, results);
    }
    size_t bytesUsed() const override {
        PYBIND11_OVERRIDE_PURE(size_t, SkBBoxHierarchy, bytesUsed);
    }
};

}  // namespace

void initPicture(py::module &m) {
py::class_<SkPicture, PyPicture, sk_sp<SkPicture>, SkRefCnt>(
    m, "Picture", R"docstring(
    :py:class:`Picture` records drawing commands made to :py:class:`Canvas`.

    The command stream may be played in whole or in part at a later time.

    :py:class:`Picture` is an abstract class. :py:class:`Picture` may be
    generated by :py:class:`PictureRecorder` or :py:class:`Drawable`, or from
    :py:class:`Picture` previously saved to :py:class:`Data` or
    :py:class:`Stream`.

    :py:class:`Picture` may contain any :py:class:`Canvas` drawing command, as
    well as one or more :py:class:`Canvas` matrix or :py:class:`Canvas` clip.
    :py:class:`Picture` has a cull :py:class:`Rect`, which is used as a bounding
    box hint. To limit :py:class:`Picture` bounds, use :py:class:`Canvas` clip
    when recording or drawing :py:class:`Picture`.

    Example::

        recorder = skia.PictureRecorder()
        canvas = recorder.beginRecording(skia.Rect(100, 100))
        canvas.clear(0xFFFFFFFF)
        canvas.drawLine(0, 0, 100, 100, skia.Paint())
        picture = recorder.finishRecordingAsPicture()
    )docstring")
    .def(py::init(&SkPicture::MakePlaceholder),
        R"docstring(
        Returns a placeholder :py:class:`Picture`.

        Result does not draw, and contains only cull :py:class:`Rect`, a hint of
        its bounds. Result is immutable; it cannot be changed later. Result
        identifier is unique.

        Returned placeholder can be intercepted during playback to insert other
        commands into :py:class:`Canvas` draw stream.

        :param skia.Rect cull: placeholder dimensions
        :return: placeholder with unique identifier
        )docstring",
        py::arg("cull"))
    .def("playback", [] (SkPicture& picture, SkCanvas* canvas) {
            picture.playback(canvas);
        },
        R"docstring(
        Replays the drawing commands on the specified canvas.

        In the case that the commands are recorded, each command in the
        :py:class:`Picture` is sent separately to canvas.

        To add a single command to draw :py:class:`Picture` to recording canvas,
        call :py:meth:`Canvas.drawPicture` instead.

        :param skia.Canvas canvas: receiver of drawing commands
        :param callback: allows interruption of playback
        )docstring",
        py::arg("canvas"))
    .def("cullRect", &SkPicture::cullRect,
        R"docstring(
        Returns cull :py:class:`Rect` for this picture, passed in when
        :py:class:`Picture` was created.

        Returned :py:class:`Rect` does not specify clipping :py:class:`Rect` for
        :py:class:`Picture`; cull is hint of :py:class:`Picture` bounds.

        :py:class:`Picture` is free to discard recorded drawing commands that
        fall outside cull.

        :return: bounds passed when :py:class:`Picture` was created
        )docstring")
    .def("uniqueID", &SkPicture::uniqueID,
        R"docstring(
        Returns a non-zero value unique among :py:class:`Picture` in Skia
        process.

        :return: identifier for :py:class:`Picture`
        )docstring")
    .def("serialize",
        [] (SkPicture& picture) { return picture.serialize(); },
        R"docstring(
        Returns storage containing :py:class:`Data` describing
        :py:class:`Picture`.

        :return: storage containing serialized :py:class:`Picture`
        )docstring")
    .def("approximateOpCount", &SkPicture::approximateOpCount,
        R"docstring(
        Returns the approximate number of operations in :py:class:`Picture`.

        Returned value may be greater or less than the number of
        :py:class:`Canvas` calls recorded: some calls may be recorded as more
        than one operation, other calls may be optimized away.

        :return: approximate operation count
        )docstring",
        py::arg("nested") = false)
    .def("approximateBytesUsed", &SkPicture::approximateBytesUsed,
        R"docstring(
        Returns the approximate byte size of :py:class:`Picture`.

        Does not include large objects referenced by :py:class:`Picture`.

        :return: approximate size
        )docstring")
    .def("makeShader",
        py::overload_cast<SkTileMode, SkTileMode, SkFilterMode, const SkMatrix*,
        const SkRect*>(&SkPicture::makeShader, py::const_),
        R"docstring(
        Return a new shader that will draw with this picture.

        :param skia.TileMode tmx: The tiling mode to use when sampling in the
            x-direction.
        :param skia.TileMode tmy: The tiling mode to use when sampling in the
            y-direction.
        :param skia.Matrix localMatrix: Optional matrix used when sampling
        :param skia.Rect tile: The tile rectangle in picture coordinates:
            this represents the subset (or superset) of the picture used when
            building a tile. It is not affected by localMatrix and does not
            imply scaling (only translation and cropping). If null, the tile
            rect is considered equal to the picture bounds.
        :return: Returns a new shader object. Note: this function never returns
            null.
        )docstring",
        py::arg("tmx"), py::arg("tmy"), py::arg("mode"), py::arg("localMatrix") = nullptr,
        py::arg("tile") = nullptr)
    .def_static("MakeFromStream",
        [] (SkStream* stream) {
            return SkPicture::MakeFromStream(stream);
        },
        R"docstring(
        Recreates :py:class:`Picture` that was serialized into a stream.

        Returns constructed :py:class:`Picture` if successful; otherwise,
        returns nullptr. Fails if data does not permit constructing valid
        :py:class:`Picture`.

        :param stream: container for serial data
        :return: :py:class:`Picture` constructed from stream data
        )docstring",
        py::arg("stream"))
    .def_static("MakeFromData",
        [] (const SkData* data) {
            auto picture = SkPicture::MakeFromData(data);
            if (!picture)
                throw py::value_error("Invalid data");
            return picture;
        },
        R"docstring(
        Recreates :py:class:`Picture` that was serialized into data.

        Returns constructed :py:class:`Picture` if successful. Fails if data
        does not permit constructing valid :py:class:`Picture`.

        :param skia.Data data: container for serial data
        :return: :py:class:`Picture` constructed from data
        :raise: ValueError
        )docstring",
        py::arg("data"))
    // .def_static("MakeFromData",
    //     py::overload_cast<const void*, size_t, const SkDeserialProcs*>(
    //         &SkPicture::MakeFromData))
    .def_static("MakePlaceholder", &SkPicture::MakePlaceholder,
        R"docstring(
        Returns a placeholder :py:class:`Picture`.

        Result does not draw, and contains only cull :py:class:`Rect`, a hint of
        its bounds. Result is immutable; it cannot be changed later. Result
        identifier is unique.

        Returned placeholder can be intercepted during playback to insert other
        commands into :py:class:`Canvas` draw stream.

        :param skia.Rect cull: placeholder dimensions
        :return: placeholder with unique identifier
        )docstring",
        py::arg("cull"))
    ;

py::class_<SkDrawable, sk_sp<SkDrawable>, SkFlattenable>(m, "Drawable",
    R"docstring(
    Base-class for objects that draw into :py:class:`Canvas`.

    The object has a generation ID, which is guaranteed to be unique across all
    drawables. To allow for clients of the drawable that may want to cache the
    results, the drawable must change its generation ID whenever its internal
    state changes such that it will draw differently.
    )docstring")
    .def("draw",
        py::overload_cast<SkCanvas*, const SkMatrix*>(&SkDrawable::draw),
        R"docstring(
        Draws into the specified content.

        The drawing sequence will be balanced upon return (i.e. the
        ``saveLevel()`` on the canvas will match what it was when
        :py:meth:`draw` was called, and the current matrix and clip settings
        will not be changed.
        )docstring",
        py::arg("canvas").none(false), py::arg("matrix") = nullptr)
    .def("draw",
        py::overload_cast<SkCanvas*, SkScalar, SkScalar>(&SkDrawable::draw),
        py::arg("canvas").none(false), py::arg("x"), py::arg("y"))
    // .def("snapGpuDrawHandler", &SkDrawable::snapGpuDrawHandler)
    .def("newPictureSnapshot", &SkDrawable::makePictureSnapshot)
    .def("getGenerationID", &SkDrawable::getGenerationID,
        R"docstring(
        Return a unique value for this instance.

        If two calls to this return the same value, it is presumed that calling
        the draw() method will render the same thing as well.

        Subclasses that change their state should call
        :py:meth:`notifyDrawingChanged` to ensure that a new value will be
        returned the next time it is called.
        )docstring")
    .def("getBounds", &SkDrawable::getBounds,
        R"docstring(
        Return the (conservative) bounds of what the drawable will draw.

        If the drawable can change what it draws (e.g. animation or in response
        to some external change), then this must return a bounds that is always
        valid for all possible states.
        )docstring")
    .def("notifyDrawingChanged", &SkDrawable::notifyDrawingChanged,
        R"docstring(
        Calling this invalidates the previous generation ID, and causes a new
        one to be computed the next time getGenerationID() is called.

        Typically this is called by the object itself, in response to its
        internal state changing.
        )docstring")
    ;

py::class_<SkBBHFactory>(m, "BBHFactory");

py::class_<SkRTreeFactory, SkBBHFactory> rTreeFactory(m, "RTreeFactory");

rTreeFactory
    .def(py::init<>())
    .def("__call__", &SkBBHFactory::operator());

py::class_<SkBBoxHierarchy, PyBBoxHierarchy, sk_sp<SkBBoxHierarchy>, SkRefCnt>
    bboxhierarchy(m, "BBoxHierarchy");

py::class_<SkBBoxHierarchy::Metadata>(bboxhierarchy, "Metadata")
    .def_readwrite("isDraw", &SkBBoxHierarchy::Metadata::isDraw);

bboxhierarchy
    .def(py::init())
    .def("insert",
        py::overload_cast<const SkRect[], int>(&SkBBoxHierarchy::insert),
        R"docstring(
        Insert N bounding boxes into the hierarchy.
        )docstring",
        py::arg("rects"), py::arg("N"))
    .def("insert",
        py::overload_cast<const SkRect[], const SkBBoxHierarchy::Metadata[],
            int>(&SkBBoxHierarchy::insert),
        py::arg("rects"), py::arg("metadata"), py::arg("N"))
    .def("search", &SkBBoxHierarchy::search,
        R"docstring(
        Populate results with the indices of bounding boxes intersecting that
        query.
        )docstring",
        py::arg("query"), py::arg("results"))
    .def("bytesUsed", &SkBBoxHierarchy::bytesUsed,
        R"docstring(
        Return approximate size in memory of this.
        )docstring")
    ;

py::class_<SkPictureRecorder> picturerecorder(m, "PictureRecorder");

/* m117: Remove slug-related #ifdefs from src/core */
/*
py::enum_<SkPictureRecorder::FinishFlags>(picturerecorder, "FinishFlags");
*/

picturerecorder
    .def(py::init())
    // .def("beginRecording",
    //     py::overload_cast<const SkRect&, sk_sp<SkBBoxHierarchy>, uint32_t>(
    //         &SkPictureRecorder::beginRecording),
    //     "Returns the canvas that records the drawing commands.")
    .def("beginRecording",
        [] (SkPictureRecorder& recorder, const SkRect& bounds) {
            return recorder.beginRecording(bounds, nullptr);
        },
        R"docstring(
        Returns the canvas that records the drawing commands.

        :bounds: the cull rect used when recording this picture. Any
            drawing the falls outside of this rect is undefined, and may be
            drawn or it may not.
        :return: the canvas.
        )docstring",
        py::arg("bounds"),
        py::return_value_policy::reference_internal)
    .def("beginRecording",
        [] (SkPictureRecorder& recorder, SkScalar width, SkScalar height) {
            return recorder.beginRecording(width, height, nullptr);
        },
        py::arg("width"), py::arg("height"),
        py::return_value_policy::reference_internal)
    .def("getRecordingCanvas", &SkPictureRecorder::getRecordingCanvas,
        R"docstring(
        Returns the recording canvas if one is active, or NULL if recording is
        not active.

        This does not alter the refcnt on the canvas (if present).
        )docstring",
        py::return_value_policy::reference_internal)
    .def("finishRecordingAsPicture",
        &SkPictureRecorder::finishRecordingAsPicture,
        R"docstring(
        Signal that the caller is done recording.

        This invalidates the canvas returned by :py:meth:`beginRecording` or
        :py:meth:`getRecordingCanvas`. Ownership of the object is passed to the
        caller, who must call unref() when they are done using it.

        The returned picture is immutable. If during recording drawables were
        added to the canvas, these will have been "drawn" into a recording
        canvas, so that this resulting picture will reflect their current state,
        but will not contain a live reference to the drawables themselves.
        )docstring")
    .def("finishRecordingAsPictureWithCull",
        &SkPictureRecorder::finishRecordingAsPictureWithCull,
        R"docstring(
        Signal that the caller is done recording, and update the cull rect to
        use for bounding box hierarchy (BBH) generation.

        The behavior is the same as calling :py:meth:`finishRecordingAsPicture`,
        except that this method updates the cull rect initially passed into
        beginRecording.

        :param skia.Rect cullRect: the new culling rectangle to use as the
            overall bound for BBH generation and subsequent culling operations.
        :return: the picture containing the recorded content.
        )docstring",
        py::arg("cullRect"))
    .def("finishRecordingAsDrawable",
        &SkPictureRecorder::finishRecordingAsDrawable,
        R"docstring(
        Signal that the caller is done recording.

        This invalidates the canvas returned by :py:meth:`beginRecording` or
        :py:meth:`getRecordingCanvas`. Ownership of the object is passed to the
        caller, who must call unref() when they are done using it.

        Unlike :py:meth:`finishRecordingAsPicture`, which returns an immutable
        picture, the returned drawable may contain live references to other
        drawables (if they were added to the recording canvas) and therefore
        this drawable will reflect the current state of those nested drawables
        anytime it is drawn or a new picture is snapped from it (by calling
        drawable.newPictureSnapshot()).
        )docstring")
    ;
}

#pragma once

#ifndef __cplusplus
#error "The Darknet/YOLO project requires a C++ compiler."
#endif

/** @file
 * This file defines @ref Darknet::Image as well as many functions which modify or create images.
 */


namespace Darknet
{
	/** Free the image.  Unlike the @p C version @ref ::free_image(), the image object is passed by reference, so the
	 * pointer to the data will be reset to @p nullptr once the memory has been freed.  When possible, prefer calling
	 * @p Darknet::free_image().
	 *
	 * All images @em must call either @ref ::free_image() or @ref Darknet::free_image() to avoid memory leaks.
	 */
	void free_image(Darknet::Image & im);

	/** Load the given image.  If both @p desired_width and @p desired_height have been set, then the image will be resized
	 * to match those dimensions.  Otherwise, specify @p 0 (zero) to leave the image dimensions unchanged.
	 *
	 * Unless single channel greyscale has been requested, the image returned will be in @em RGB format, not @em BGR.
	 *
	 * @note Remember to call @ref Darknet::free_image() once an image is no longer needed.
	 *
	 * @see @ref Darknet::image_to_mat()
	 * @see @ref Darknet::mat_to_image()
	 */
	Darknet::Image load_image(const char * filename, int desired_width = 0, int desired_height = 0, int channels = 0);

	/// Make an empty image with the given dimensions.  The data pointer will be @p nullptr.  @see @ref Darknet::make_image()
	Darknet::Image make_empty_image(int w, int h, int c);

	/// Similar to @ref make_empty_image() but the data pointer is fully allocated.
	Darknet::Image make_image(int w, int h, int c);

	/** Convert an OpenCV @p cv::Mat object to @ref Darknet::Image.  The @p cv::Mat is expected to already have been
	 * converted from @p BGR to @p RGB.  The result @ref Darknet::Image floats will be normalized between @p 0.0 and @p 1.0.
	 *
	 * @see @p cv::COLOR_BGR2RGB
	 * @see @ref image_to_mat()
	 */
	Darknet::Image mat_to_image(const cv::Mat & mat);

	/** Convert the usual @ref Darknet::Image format to OpenCV @p cv::Mat.  The mat object will be in @p RGB format,
	 * not @p BGR.
	 *
	 * @see @p cv::COLOR_RGB2BGR
	 * @see @ref mat_to_image()
	 */
	cv::Mat image_to_mat(const Darknet::Image & img);

	/// Generate some "random" colour value to use.  Mostly used for labels and charts.
	float get_color(int c, int x, int max);

	/// Flip image left <-> right.
	void flip_image(Darknet::Image & a);

	/// Draw a bounding box at the rectangle coordinates within an image, using the specified colour.
	void draw_box(Darknet::Image & a, int x1, int y1, int x2, int y2, float r, float g, float b);

	/// Similiar to @ref Darknet::draw_box(), but the line thickness can be specified using @p w.
	void draw_box_width(Darknet::Image & a, int x1, int y1, int x2, int y2, int w, float r, float g, float b);

	/// Draw the given bounding box.  The line thickness can be specified using @p w.
	void draw_bbox(Darknet::Image & a, const Darknet::Box & bbox, int w, float r, float g, float b);

	/// Blend the label (actually an image) into the given image.
	void draw_weighted_label(Darknet::Image & a, int r, int c, Darknet::Image & label, const float *rgb, const float alpha);

	/// This is the function that is called from older %Darknet code to draw annotations on an image.
	void draw_detections_v3(Darknet::Image & im, const Darknet::Detection * dets, const int num, const float thresh, const Darknet::VStr & names, const int classes, const int ext_output);

	/// Draw a bounding box on a black-and-white image.
	void draw_box_bw(Darknet::Image & a, int x1, int y1, int x2, int y2, float brightness);

	/// Similar to @ref Darknet::draw_box_bw(), but the line thickness can be specified using @p w.
	void draw_box_width_bw(Darknet::Image & a, int x1, int y1, int x2, int y2, int w, float brightness);

	/** Create a single-channel image for ...?
	 *
	 * @note Function currently seems to be unused.
	 */
	Darknet::Image image_distance(Darknet::Image & a, Darknet::Image & b);

	/// Scale the RGB values in an image by the given amount.
	void scale_image(Darknet::Image & m, const float s);

	/// Crop the given image.  @see @ref Darknet::random_crop_image()
	Darknet::Image crop_image(const Darknet::Image & im, const int dx, const int dy, const int w, const int h);

	/// Similar to @ref Darknet::crop_image() but the @p dx and @p dy are random values.
	Darknet::Image random_crop_image(const Darknet::Image & im, const int w, const int h);

	/** Apply a bunch of random augmentations.
	 *
	 * @note Function currently seems to be unused.
	 */
	Darknet::Image random_augment_image(const Darknet::Image & im, const float angle, float aspect, const int low, const int high, const int size);

	/** Rotate image clockwise?
	 *
	 * @note Function currently seems to be unused.
	 */
	void rotate_image_cw(Darknet::Image & im, int times);

	/** Rotate image.
	 *
	 * @note Function currently seems to be unused.
	 */
	Darknet::Image rotate_image(const Darknet::Image & im, float rad);

	Darknet::Image rotate_crop_image(const Darknet::Image & im, float rad, float s, int w, int h, float dx, float dy, float aspect);

	/// Do the equivalent of OpenCV's @p cv::COLOR_BGR2RGB to swap red and blue floats.
	void rgbgr_image(Darknet::Image & im);

	Darknet::Image resize_image(const Darknet::Image & im, int w, int h);

	/// @note Function currently seems to be unused.
	Darknet::Image resize_min(const Darknet::Image & im, int min);

	/// @note Function currently seems to be unused.
	Darknet::Image resize_max(const Darknet::Image & im, int max);

	void make_image_red(Darknet::Image & im);

	void constrain_image(Darknet::Image & im);

	void normalize_image(Darknet::Image & p);

	/// @note Function currently seems to be unused.
	void normalize_image2(Darknet::Image & p);

	/// @note Function currently seems to be unused.
	void quantize_image(Darknet::Image & im);

	/// The resulting image takes ownership of @p original_delta_cpu.
	Darknet::Image make_attention_image(int img_size, float *original_delta_cpu, float *original_input_cpu, int w, int h, int c, float alpha);

	/// Return a specific channel (eg: R, G, B) from an image.
	Darknet::Image get_image_layer(const Darknet::Image & m, int l);

	void fill_image(Darknet::Image & m, float s);

	void embed_image(const Darknet::Image & source, Darknet::Image & dest, int dx, int dy);

	Darknet::Image collapse_image_layers(const Darknet::Image & source, int border);

	/// @note Function currently seems to be unused.
	void copy_image_inplace(const Darknet::Image & src, Darknet::Image & dst);

	Darknet::Image copy_image(const Darknet::Image & p);

	/// @note Function currently seems to be unused.
	void rgb_to_hsv(Darknet::Image & im);

	void hsv_to_rgb(Darknet::Image & im);

	void copy_image_from_bytes(Darknet::Image im, char *pdata);
	Darknet::Image letterbox_image(const Darknet::Image & im, int w, int h);
	void letterbox_image_into(const Darknet::Image & im, int w, int h, Darknet::Image & boxed);
	void random_distort_image(Darknet::Image & im, float hue, float saturation, float exposure);
	void translate_image(Darknet::Image m, float s);
	void distort_image(Darknet::Image & im, float hue, float sat, float val);
	void composite_image(const Darknet::Image & source, Darknet::Image & dest, int dx, int dy);
	void composite_3d(char *f1, char *f2, char *out, int delta);
	int best_3d_shift_r(const Darknet::Image & a, const Darknet::Image & b, int min, int max);
	int best_3d_shift(const Darknet::Image & a, const Darknet::Image & b, int min, int max);

	Darknet::Image grayscale_image(const Darknet::Image & im);
	Darknet::Image threshold_image(const Darknet::Image & im, float thresh);
	Darknet::Image blend_image(const Darknet::Image & fore, const Darknet::Image & back, float alpha);
	void scale_image_channel(Darknet::Image & im, int c, float v);
	Darknet::Image collapse_images_horz(const Darknet::Image *ims, int n);
	Darknet::Image collapse_images_vert(const Darknet::Image *ims, int n);

	void save_image(const Darknet::Image & p, const char * name);
	void save_image_png(const Darknet::Image & im, const char * name);
	void save_image_jpg(const Darknet::Image & im, const char * name);

	void show_image(const Darknet::Image & p, const char * name);
	void show_images(Darknet::Image *ims, int n, const char * window);
	void show_image_layers(const Darknet::Image & p, const char * name);
	void show_image_collapsed(const Darknet::Image & p, const char * name);

	void print_image(const Darknet::Image & m);

	Darknet::Image make_random_image(int w, int h, int c);
	Darknet::Image float_to_image_scaled(int w, int h, int c, float *data);
	Darknet::Image float_to_image(int w, int h, int c, float *data);

	float bilinear_interpolate(const Darknet::Image & im, float x, float y, int c);

	void test_resize(char *filename);
}

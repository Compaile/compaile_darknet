// Gaussian YOLOv3 implementation
// Author: Jiwoong Choi
// ICCV 2019 Paper: http://openaccess.thecvf.com/content_ICCV_2019/html/Choi_Gaussian_YOLOv3_An_Accurate_and_Fast_Object_Detector_Using_Localization_ICCV_2019_paper.html
// arxiv.org: https://arxiv.org/abs/1904.04620v2
// source code: https://github.com/jwchoi384/Gaussian_YOLOv3

#include "darknet_internal.hpp"

namespace
{
	static auto & cfg_and_state = Darknet::CfgAndState::get();


	inline Darknet::Box get_gaussian_yolo_box(const float * x, const float * biases, const int n, const int index, const int i, const int j, const int lw, const int lh, const int w, const int h, const int stride, const YOLO_POINT yolo_point)
	{
		TAT_COMMENT(TATPARMS, "2024-05-14 inlined");

		Darknet::Box b;

		b.w = exp(x[index + 4 * stride]) * biases[2 * n] / w;
		b.h = exp(x[index + 6 * stride]) * biases[2 * n + 1] / h;
		b.x = (i + x[index + 0 * stride]) / lw;
		b.y = (j + x[index + 2 * stride]) / lh;

		if (yolo_point == YOLO_CENTER)
		{
		}
		else if (yolo_point == YOLO_LEFT_TOP)
		{
			b.x = (i + x[index + 0 * stride]) / lw + b.w / 2;
			b.y = (j + x[index + 2 * stride]) / lh + b.h / 2;
		}
		else if (yolo_point == YOLO_RIGHT_BOTTOM)
		{
			b.x = (i + x[index + 0 * stride]) / lw - b.w / 2;
			b.y = (j + x[index + 2 * stride]) / lh - b.h / 2;
		}

		return b;
	}


	inline void fix_nan_inf(float & val)
	{
		TAT_COMMENT(TATPARMS, "2024-05-14 inlined");

		if (std::isnan(val) or
			std::isinf(val))
		{
			val = 0.0f;
		}

		return;
	}


	inline void clip_value(float & val, const float max_val)
	{
		TAT_COMMENT(TATPARMS, "2024-05-14 inlined");

		if (val > max_val)
		{
			val = max_val;
		}
		else if (val < -max_val)
		{
			val = -max_val;
		}

		return;
	}


	inline float delta_gaussian_yolo_box(const Darknet::Box & truth, const float * x, const float * biases, const int n, const int index, const int i, const int j, const int lw, const int lh, const int w, const int h, float * delta,
										 const float scale, const int stride, const float iou_normalizer, const IOU_LOSS iou_loss, const float uc_normalizer, const int accumulate, const YOLO_POINT yolo_point, const float max_delta)
	{
		TAT_COMMENT(TATPARMS, "2024-05-14 inlined");

		Darknet::Box pred = get_gaussian_yolo_box(x, biases, n, index, i, j, lw, lh, w, h, stride, yolo_point);

		float iou;
		ious all_ious = { 0 };
		all_ious.iou = box_iou(pred, truth);
		all_ious.giou = box_giou(pred, truth);
		all_ious.diou = box_diou(pred, truth);
		all_ious.ciou = box_ciou(pred, truth);
		if (pred.w == 0) { pred.w = 1.0; }
		if (pred.h == 0) { pred.h = 1.0; }

		float sigma_const = 0.3;
		float epsi = pow(10,-9);

		iou = all_ious.iou;

		float tx = (truth.x*lw - i);
		float ty = (truth.y*lh - j);
		float tw = log(truth.w*w / biases[2 * n]);
		float th = log(truth.h*h / biases[2 * n + 1]);

		if (yolo_point == YOLO_CENTER)
		{
		}
		else if (yolo_point == YOLO_LEFT_TOP)
		{
			tx = ((truth.x - truth.w / 2)*lw - i);
			ty = ((truth.y - truth.h / 2)*lh - j);
		}
		else if (yolo_point == YOLO_RIGHT_BOTTOM)
		{
			tx = ((truth.x + truth.w / 2)*lw - i);
			ty = ((truth.y + truth.h / 2)*lh - j);
		}

		float dx2 = (tx - x[index + 0 * stride]);
		float dy2 = (ty - x[index + 2 * stride]);
		float dw2 = (tw - x[index + 4 * stride]);
		float dh2 = (th - x[index + 6 * stride]);

		// Gaussian
		float in_exp_x = dx2 / x[index+1*stride];
		float in_exp_x_2 = pow(in_exp_x, 2);
		float normal_dist_x = exp(in_exp_x_2*(-1./2.))/(sqrt(M_PI * 2.0)*(x[index+1*stride]+sigma_const));

		float in_exp_y = dy2 / x[index+3*stride];
		float in_exp_y_2 = pow(in_exp_y, 2);
		float normal_dist_y = exp(in_exp_y_2*(-1./2.))/(sqrt(M_PI * 2.0)*(x[index+3*stride]+sigma_const));

		float in_exp_w = dw2 / x[index+5*stride];
		float in_exp_w_2 = pow(in_exp_w, 2);
		float normal_dist_w = exp(in_exp_w_2*(-1./2.))/(sqrt(M_PI * 2.0)*(x[index+5*stride]+sigma_const));

		float in_exp_h = dh2 / x[index+7*stride];
		float in_exp_h_2 = pow(in_exp_h, 2);
		float normal_dist_h = exp(in_exp_h_2*(-1./2.))/(sqrt(M_PI * 2.0)*(x[index+7*stride]+sigma_const));

		float temp_x = (1.0f/2.0f) * 1.0f/(normal_dist_x+epsi) * normal_dist_x * scale;
		float temp_y = (1.0f/2.0f) * 1.0f/(normal_dist_y+epsi) * normal_dist_y * scale;
		float temp_w = (1.0f/2.0f) * 1.0f/(normal_dist_w+epsi) * normal_dist_w * scale;
		float temp_h = (1.0f/2.0f) * 1.0f/(normal_dist_h+epsi) * normal_dist_h * scale;

		if (!accumulate)
		{
			delta[index + 0 * stride] = 0.0f;
			delta[index + 1 * stride] = 0.0f;
			delta[index + 2 * stride] = 0.0f;
			delta[index + 3 * stride] = 0.0f;
			delta[index + 4 * stride] = 0.0f;
			delta[index + 5 * stride] = 0.0f;
			delta[index + 6 * stride] = 0.0f;
			delta[index + 7 * stride] = 0.0f;
		}

		float delta_x = temp_x * in_exp_x  * (1.0f / x[index + 1 * stride]);
		float delta_y = temp_y * in_exp_y  * (1.0f / x[index + 3 * stride]);
		float delta_w = temp_w * in_exp_w  * (1.0f / x[index + 5 * stride]);
		float delta_h = temp_h * in_exp_h  * (1.0f / x[index + 7 * stride]);

		float delta_ux = temp_x * (in_exp_x_2 / x[index + 1 * stride] - 1.0f / (x[index + 1 * stride] + sigma_const));
		float delta_uy = temp_y * (in_exp_y_2 / x[index + 3 * stride] - 1.0f / (x[index + 3 * stride] + sigma_const));
		float delta_uw = temp_w * (in_exp_w_2 / x[index + 5 * stride] - 1.0f / (x[index + 5 * stride] + sigma_const));
		float delta_uh = temp_h * (in_exp_h_2 / x[index + 7 * stride] - 1.0f / (x[index + 7 * stride] + sigma_const));

		if (iou_loss != MSE)
		{
			// GIoU
			iou = all_ious.giou;

			// https://github.com/generalized-iou/g-darknet
			// https://arxiv.org/abs/1902.09630v2
			// https://giou.stanford.edu/
			// https://arxiv.org/abs/1911.08287v1
			// https://github.com/Zzh-tju/DIoU-darknet
			all_ious.dx_iou = dx_box_iou(pred, truth, iou_loss);

			float dx = all_ious.dx_iou.dt;
			float dy = all_ious.dx_iou.db;
			float dw = all_ious.dx_iou.dl;
			float dh = all_ious.dx_iou.dr;

			if (yolo_point == YOLO_CENTER)
			{
			}
			else if (yolo_point == YOLO_LEFT_TOP)
			{
				dx = dx - dw/2;
				dy = dy - dh/2;
			}
			else if (yolo_point == YOLO_RIGHT_BOTTOM)
			{
				dx = dx + dw / 2;
				dy = dy + dh / 2;
			}

			// jacobian^t (transpose)
			//float dx = (all_ious.dx_iou.dl + all_ious.dx_iou.dr);
			//float dy = (all_ious.dx_iou.dt + all_ious.dx_iou.db);
			//float dw = ((-0.5 * all_ious.dx_iou.dl) + (0.5 * all_ious.dx_iou.dr));
			//float dh = ((-0.5 * all_ious.dx_iou.dt) + (0.5 * all_ious.dx_iou.db));

			// predict exponential, apply gradient of e^delta_t ONLY for w,h
			dw *= exp(x[index + 4 * stride]);
			dh *= exp(x[index + 6 * stride]);

			delta_x = dx;
			delta_y = dy;
			delta_w = dw;
			delta_h = dh;
		}

		// normalize iou weight, for GIoU
		delta_x *= iou_normalizer;
		delta_y *= iou_normalizer;
		delta_w *= iou_normalizer;
		delta_h *= iou_normalizer;

		// normalize Uncertainty weight
		delta_ux *= uc_normalizer;
		delta_uy *= uc_normalizer;
		delta_uw *= uc_normalizer;
		delta_uh *= uc_normalizer;

		fix_nan_inf(delta_x);
		fix_nan_inf(delta_y);
		fix_nan_inf(delta_w);
		fix_nan_inf(delta_h);

		fix_nan_inf(delta_ux);
		fix_nan_inf(delta_uy);
		fix_nan_inf(delta_uw);
		fix_nan_inf(delta_uh);

		if (max_delta != FLT_MAX)
		{
			clip_value(delta_x, max_delta);
			clip_value(delta_y, max_delta);
			clip_value(delta_w, max_delta);
			clip_value(delta_h, max_delta);

			clip_value(delta_ux, max_delta);
			clip_value(delta_uy, max_delta);
			clip_value(delta_uw, max_delta);
			clip_value(delta_uh, max_delta);
		}

		delta[index + 0 * stride] += delta_x;
		delta[index + 2 * stride] += delta_y;
		delta[index + 4 * stride] += delta_w;
		delta[index + 6 * stride] += delta_h;

		delta[index + 1 * stride] += delta_ux;
		delta[index + 3 * stride] += delta_uy;
		delta[index + 5 * stride] += delta_uw;
		delta[index + 7 * stride] += delta_uh;

		return iou;
	}


	inline void averages_gaussian_yolo_deltas(const int class_index, const int box_index, const int stride, const int classes, float * delta)
	{
		TAT_COMMENT(TATPARMS, "2024-05-14 inlined");

		int classes_in_one_box = 0;
		for (int c = 0; c < classes; ++c)
		{
			if (delta[class_index + stride*c] > 0)
			{
				classes_in_one_box++;
			}
		}

		if (classes_in_one_box > 0)
		{
			delta[box_index + 0 * stride] /= classes_in_one_box;
			delta[box_index + 1 * stride] /= classes_in_one_box;
			delta[box_index + 2 * stride] /= classes_in_one_box;
			delta[box_index + 3 * stride] /= classes_in_one_box;
			delta[box_index + 4 * stride] /= classes_in_one_box;
			delta[box_index + 5 * stride] /= classes_in_one_box;
			delta[box_index + 6 * stride] /= classes_in_one_box;
			delta[box_index + 7 * stride] /= classes_in_one_box;
		}
	}


	inline void delta_gaussian_yolo_class(const float * output, float * delta, const int index, const int class_id, const int classes, const int stride, float * avg_cat, const float label_smooth_eps, const float * classes_multipliers, const float cls_normalizer)
	{
		TAT_COMMENT(TATPARMS, "2024-05-14 inlined");

		if (delta[index])
		{
			float y_true = 1;
			if (label_smooth_eps)
			{
				y_true = y_true *  (1 - label_smooth_eps) + 0.5f * label_smooth_eps;
			}
			delta[index + stride*class_id] = y_true - output[index + stride*class_id];

			if (classes_multipliers)
			{
				delta[index + stride*class_id] *= classes_multipliers[class_id];
			}

			if (avg_cat)
			{
				*avg_cat += output[index + stride*class_id];
			}

			return;
		}

		for(int n = 0; n < classes; ++n)
		{
			float y_true = ((n == class_id) ? 1 : 0);
			if (label_smooth_eps)
			{
				y_true = y_true *  (1 - label_smooth_eps) + 0.5 * label_smooth_eps;
			}
			delta[index + stride*n] = y_true - output[index + stride*n];

			if (classes_multipliers && n == class_id)
			{
				delta[index + stride*class_id] *= classes_multipliers[class_id] * cls_normalizer;
			}

			if (n == class_id && avg_cat)
			{
				*avg_cat += output[index + stride*n];
			}
		}
	}


	inline int compare_gaussian_yolo_class(const float *output, const int classes, const int class_index, const int stride, const float objectness, const int class_id, const float conf_thresh)
	{
		TAT_COMMENT(TATPARMS, "2024-05-14 inlined");

		for (int j = 0; j < classes; ++j)
		{
			const float prob = output[class_index + stride*j];
			if (prob > conf_thresh)
			{
				return 1;
			}
		}

		return 0;
	}


	inline int entry_gaussian_index(const Darknet::Layer & l, const int batch, const int location, const int entry)
	{
		TAT_COMMENT(TATPARMS, "2024-05-14 inlined");

		int n =   location / (l.w*l.h);
		int loc = location % (l.w*l.h);

		return batch * l.outputs + n * l.w * l.h * (8 + l.classes + 1) + entry * l.w * l.h + loc;
	}

} // anonymous namespace


Darknet::Layer make_gaussian_yolo_layer(int batch, int w, int h, int n, int total, int *mask, int classes, int max_boxes)
{
	TAT(TATPARMS);

	int i;
	Darknet::Layer l = { (Darknet::ELayerType)0 };
	l.type = Darknet::ELayerType::GAUSSIAN_YOLO;

	l.n = n;
	l.total = total;
	l.batch = batch;
	l.h = h;
	l.w = w;
	l.c = n*(classes + 8 + 1);
	l.out_w = l.w;
	l.out_h = l.h;
	l.out_c = l.c;
	l.classes = classes;
	l.cost = (float*)calloc(1, sizeof(float));
	l.biases = (float*)calloc(total*2, sizeof(float));
	if (mask)
	{
		l.mask = mask;
	}
	else
	{
		l.mask = (int*)calloc(n, sizeof(int));
		for (i = 0; i < n; ++i)
		{
			l.mask[i] = i;
		}
	}
	l.bias_updates = (float*)calloc(n*2, sizeof(float));
	l.outputs = h*w*n*(classes + 8 + 1);
	l.inputs = l.outputs;
	l.max_boxes = max_boxes;
	l.truth_size = 4 + 2;
	l.truths = l.max_boxes*l.truth_size;
	l.delta = (float*)calloc(batch*l.outputs, sizeof(float));
	l.output = (float*)calloc(batch*l.outputs, sizeof(float));
	for(i = 0; i < total*2; ++i){
		l.biases[i] = .5;
	}

	l.forward = forward_gaussian_yolo_layer;
	l.backward = backward_gaussian_yolo_layer;
#ifdef DARKNET_GPU
	l.forward_gpu = forward_gaussian_yolo_layer_gpu;
	l.backward_gpu = backward_gaussian_yolo_layer_gpu;
	l.output_gpu = cuda_make_array(l.output, batch*l.outputs);
	l.delta_gpu = cuda_make_array(l.delta, batch*l.outputs);


	free(l.output);
	if (cudaSuccess == cudaHostAlloc((void**)&l.output, batch*l.outputs * sizeof(float), cudaHostRegisterMapped)) l.output_pinned = 1;
	else {
		std::ignore = cudaGetLastError(); // reset CUDA-error
		l.output = (float*)calloc(batch * l.outputs, sizeof(float));
	}

	free(l.delta);
	if (cudaSuccess == cudaHostAlloc((void**)&l.delta, batch*l.outputs * sizeof(float), cudaHostRegisterMapped)) l.delta_pinned = 1;
	else {
		std::ignore = cudaGetLastError(); // reset CUDA-error
		l.delta = (float*)calloc(batch * l.outputs, sizeof(float));
	}

#endif

	return l;
}

void resize_gaussian_yolo_layer(Darknet::Layer *l, int w, int h)
{
	TAT(TATPARMS);

	l->w = w;
	l->h = h;

	l->outputs = h*w*l->n*(l->classes + 8 + 1);
	l->inputs = l->outputs;

	//l->output = (float *)realloc(l->output, l->batch*l->outputs * sizeof(float));
	//l->delta = (float *)realloc(l->delta, l->batch*l->outputs * sizeof(float));

	if (!l->output_pinned) l->output = (float*)realloc(l->output, l->batch*l->outputs * sizeof(float));
	if (!l->delta_pinned) l->delta = (float*)realloc(l->delta, l->batch*l->outputs * sizeof(float));

#ifdef DARKNET_GPU

	if (l->output_pinned) {
		CHECK_CUDA(cudaFreeHost(l->output));
		if (cudaSuccess != cudaHostAlloc((void**)&l->output, l->batch*l->outputs * sizeof(float), cudaHostRegisterMapped)) {
			std::ignore = cudaGetLastError(); // reset CUDA-error
			l->output = (float*)calloc(l->batch * l->outputs, sizeof(float));
			l->output_pinned = 0;
		}
	}

	if (l->delta_pinned) {
		CHECK_CUDA(cudaFreeHost(l->delta));
		if (cudaSuccess != cudaHostAlloc((void**)&l->delta, l->batch*l->outputs * sizeof(float), cudaHostRegisterMapped)) {
			std::ignore = cudaGetLastError(); // reset CUDA-error
			l->delta = (float*)calloc(l->batch * l->outputs, sizeof(float));
			l->delta_pinned = 0;
		}
	}


	cuda_free(l->delta_gpu);
	cuda_free(l->output_gpu);

	l->delta_gpu =     cuda_make_array(l->delta, l->batch*l->outputs);
	l->output_gpu =    cuda_make_array(l->output, l->batch*l->outputs);
#endif
}


void forward_gaussian_yolo_layer(Darknet::Layer & l, Darknet::NetworkState state)
{
	TAT(TATPARMS);

	int i,j,b,t,n;
	memcpy(l.output, state.input, l.outputs*l.batch*sizeof(float));

#ifndef DARKNET_GPU
	for (b = 0; b < l.batch; ++b)
	{
		for(n = 0; n < l.n; ++n)
		{
			// x : mu, sigma
			int index = entry_gaussian_index(l, b, n*l.w*l.h, 0);
			activate_array(l.output + index, 2*l.w*l.h, LOGISTIC);
			scal_add_cpu(l.w*l.h, l.scale_x_y, -0.5*(l.scale_x_y - 1), l.output + index, 1);    // scale x
			// y : mu, sigma
			index = entry_gaussian_index(l, b, n*l.w*l.h, 2);
			activate_array(l.output + index, 2*l.w*l.h, LOGISTIC);
			scal_add_cpu(l.w*l.h, l.scale_x_y, -0.5*(l.scale_x_y - 1), l.output + index, 1);    // scale y
			// w : sigma
			index = entry_gaussian_index(l, b, n*l.w*l.h, 5);
			activate_array(l.output + index, l.w*l.h, LOGISTIC);
			// h : sigma
			index = entry_gaussian_index(l, b, n*l.w*l.h, 7);
			activate_array(l.output + index, l.w*l.h, LOGISTIC);
			// objectness & class
			index = entry_gaussian_index(l, b, n*l.w*l.h, 8);
			activate_array(l.output + index, (1+l.classes)*l.w*l.h, LOGISTIC);
		}
	}
#endif

	memset(l.delta, 0, l.outputs * l.batch * sizeof(float));
	if (!state.train) return;
	float avg_iou = 0;
	float recall = 0;
	float recall75 = 0;
	float avg_cat = 0;
	float avg_obj = 0;
	float avg_anyobj = 0;
	int count = 0;
	int class_count = 0;
	*(l.cost) = 0;
	for (b = 0; b < l.batch; ++b)
	{
		for (j = 0; j < l.h; ++j)
		{
			for (i = 0; i < l.w; ++i)
			{
				for (n = 0; n < l.n; ++n)
				{
					const int class_index = entry_gaussian_index(l, b, n*l.w*l.h + j*l.w + i, 9);
					const int obj_index = entry_gaussian_index(l, b, n*l.w*l.h + j*l.w + i, 8);
					const int box_index = entry_gaussian_index(l, b, n*l.w*l.h + j*l.w + i, 0);
					const int stride = l.w*l.h;
					Darknet::Box pred = get_gaussian_yolo_box(l.output, l.biases, l.mask[n], box_index, i, j, l.w, l.h, state.net.w, state.net.h, l.w*l.h, l.yolo_point);
					float best_match_iou = 0;
					int best_match_t = 0;
					float best_iou = 0;
					int best_t = 0;
					for(t = 0; t < l.max_boxes; ++t)
					{
						Darknet::Box truth = float_to_box_stride(state.truth + t*l.truth_size + b*l.truths, 1);
						int class_id = state.truth[t*l.truth_size + b*l.truths + 4];
						if (class_id >= l.classes)
						{
							darknet_fatal_error(DARKNET_LOC, "invalid class ID #%d", class_id);
						}

						if(!truth.x)
						{
							break;
						}

						float objectness = l.output[obj_index];
						int class_id_match = compare_gaussian_yolo_class(l.output, l.classes, class_index, l.w*l.h, objectness, class_id, 0.25f);

						float iou = box_iou(pred, truth);
						if (iou > best_match_iou && class_id_match == 1)
						{
							best_match_iou = iou;
							best_match_t = t;
						}
						if (iou > best_iou)
						{
							best_iou = iou;
							best_t = t;
						}
					}

					avg_anyobj += l.output[obj_index];
					l.delta[obj_index] = l.obj_normalizer * (0 - l.output[obj_index]);
					if (best_match_iou > l.ignore_thresh)
					{
						const float iou_multiplier = best_match_iou*best_match_iou;// (best_match_iou - l.ignore_thresh) / (1.0 - l.ignore_thresh);
						if (l.objectness_smooth)
						{
							l.delta[obj_index] = l.obj_normalizer * (iou_multiplier - l.output[obj_index]);

							int class_id = state.truth[best_match_t*l.truth_size + b*l.truths + 4];
							if (l.map)
							{
								class_id = l.map[class_id];
							}
							delta_gaussian_yolo_class(l.output, l.delta, class_index, class_id, l.classes, l.w*l.h, 0, l.label_smooth_eps, l.classes_multipliers, l.cls_normalizer);
						}
						else
						{
							l.delta[obj_index] = 0;
						}
					}
					else if (state.net.adversarial)
					{
						float scale = pred.w * pred.h;
						if (scale > 0) scale = sqrt(scale);
						l.delta[obj_index] = scale * l.obj_normalizer * (0 - l.output[obj_index]);
						int cl_id;
						for (cl_id = 0; cl_id < l.classes; ++cl_id)
						{
							if (l.output[class_index + stride*cl_id] * l.output[obj_index] > 0.25)
							{
								l.delta[class_index + stride*cl_id] = scale * (0 - l.output[class_index + stride*cl_id]);
							}
						}
					}
					if (best_iou > l.truth_thresh)
					{
						const float iou_multiplier = best_iou*best_iou;// (best_iou - l.truth_thresh) / (1.0 - l.truth_thresh);
						if (l.objectness_smooth)
						{
							l.delta[obj_index] = l.obj_normalizer * (iou_multiplier - l.output[obj_index]);
						}
						else
						{
							l.delta[obj_index] = l.obj_normalizer * (1 - l.output[obj_index]);
						}
						//l.delta[obj_index] = l.obj_normalizer * (1 - l.output[obj_index]);

						int class_id = state.truth[best_t*l.truth_size + b*l.truths + 4];
						if (l.map)
						{
							class_id = l.map[class_id];
						}
						delta_gaussian_yolo_class(l.output, l.delta, class_index, class_id, l.classes, l.w*l.h, 0, l.label_smooth_eps, l.classes_multipliers, l.cls_normalizer);
						const float class_multiplier = (l.classes_multipliers) ? l.classes_multipliers[class_id] : 1.0f;
						if (l.objectness_smooth)
						{
							l.delta[class_index + stride*class_id] = class_multiplier * (iou_multiplier - l.output[class_index + stride*class_id]);
						}
						Darknet::Box truth = float_to_box_stride(state.truth + best_t*l.truth_size + b*l.truths, 1);
						delta_gaussian_yolo_box(truth, l.output, l.biases, l.mask[n], box_index, i, j, l.w, l.h, state.net.w, state.net.h, l.delta, (2-truth.w*truth.h), l.w*l.h, l.iou_normalizer * class_multiplier, l.iou_loss, l.uc_normalizer, 1, l.yolo_point, l.max_delta);
					}
				}
			}
		}
		for(t = 0; t < l.max_boxes; ++t)
		{
			Darknet::Box truth = float_to_box_stride(state.truth + t*l.truth_size + b*l.truths, 1);

			if(!truth.x)
			{
				break;
			}

			float best_iou = 0;
			int best_n = 0;
			i = (truth.x * l.w);
			j = (truth.y * l.h);

			if (l.yolo_point == YOLO_CENTER)
			{
			}
			else if (l.yolo_point == YOLO_LEFT_TOP)
			{
				i = min_val_cmp(l.w-1, max_val_cmp(0, ((truth.x - truth.w / 2) * l.w)));
				j = min_val_cmp(l.h-1, max_val_cmp(0, ((truth.y - truth.h / 2) * l.h)));
			}
			else if (l.yolo_point == YOLO_RIGHT_BOTTOM)
			{
				i = min_val_cmp(l.w-1, max_val_cmp(0, ((truth.x + truth.w / 2) * l.w)));
				j = min_val_cmp(l.h-1, max_val_cmp(0, ((truth.y + truth.h / 2) * l.h)));
			}

			Darknet::Box truth_shift = truth;
			truth_shift.x = truth_shift.y = 0;
			for(n = 0; n < l.total; ++n)
			{
				Darknet::Box pred = {0};
				pred.w = l.biases[2*n]/ state.net.w;
				pred.h = l.biases[2*n+1]/ state.net.h;
				float iou = box_iou(pred, truth_shift);
				if (iou > best_iou)
				{
					best_iou = iou;
					best_n = n;
				}
			}

			int mask_n2 = int_index(l.mask, best_n, l.n);
			if(mask_n2 >= 0)
			{
				int class_id = state.truth[t*l.truth_size + b*l.truths + 4];
				if (l.map)
				{
					class_id = l.map[class_id];
				}

				int box_index = entry_gaussian_index(l, b, mask_n2*l.w*l.h + j*l.w + i, 0);
				const float class_multiplier = (l.classes_multipliers) ? l.classes_multipliers[class_id] : 1.0f;
				float iou = delta_gaussian_yolo_box(truth, l.output, l.biases, best_n, box_index, i, j, l.w, l.h, state.net.w, state.net.h, l.delta, (2-truth.w*truth.h), l.w*l.h, l.iou_normalizer * class_multiplier, l.iou_loss, l.uc_normalizer, 1, l.yolo_point, l.max_delta);

				int obj_index = entry_gaussian_index(l, b, mask_n2*l.w*l.h + j*l.w + i, 8);
				avg_obj += l.output[obj_index];
				l.delta[obj_index] = class_multiplier * l.obj_normalizer * (1 - l.output[obj_index]);

				int class_index = entry_gaussian_index(l, b, mask_n2*l.w*l.h + j*l.w + i, 9);
				delta_gaussian_yolo_class(l.output, l.delta, class_index, class_id, l.classes, l.w*l.h, &avg_cat, l.label_smooth_eps, l.classes_multipliers, l.cls_normalizer);

				++count;
				++class_count;
				if(iou > 0.5f)
				{
					recall += 1;
				}
				if(iou > 0.75f)
				{
					recall75 += 1;
				}
				avg_iou += iou;
			}


			// iou_thresh
			for (n = 0; n < l.total; ++n)
			{
				int mask_n = int_index(l.mask, n, l.n);
				if (mask_n >= 0 && n != best_n && l.iou_thresh < 1.0f)
				{
					Darknet::Box pred = { 0 };
					pred.w = l.biases[2 * n] / state.net.w;
					pred.h = l.biases[2 * n + 1] / state.net.h;
					float box_iou = box_iou_kind(pred, truth_shift, l.iou_thresh_kind); // IOU, GIOU, MSE, DIOU, CIOU
					// iou, n

					if (box_iou > l.iou_thresh)
					{
						int class_id = state.truth[t*l.truth_size + b*l.truths + 4];
						if (l.map)
						{
							class_id = l.map[class_id];
						}

						int box_index = entry_gaussian_index(l, b, mask_n*l.w*l.h + j*l.w + i, 0);
						const float class_multiplier = (l.classes_multipliers) ? l.classes_multipliers[class_id] : 1.0f;
						float iou = delta_gaussian_yolo_box(truth, l.output, l.biases, n, box_index, i, j, l.w, l.h, state.net.w, state.net.h, l.delta, (2 - truth.w*truth.h), l.w*l.h, l.iou_normalizer * class_multiplier, l.iou_loss, l.uc_normalizer, 1, l.yolo_point, l.max_delta);

						int obj_index = entry_gaussian_index(l, b, mask_n*l.w*l.h + j*l.w + i, 8);
						avg_obj += l.output[obj_index];
						l.delta[obj_index] = class_multiplier * l.obj_normalizer * (1 - l.output[obj_index]);

						int class_index = entry_gaussian_index(l, b, mask_n*l.w*l.h + j*l.w + i, 9);
						delta_gaussian_yolo_class(l.output, l.delta, class_index, class_id, l.classes, l.w*l.h, &avg_cat, l.label_smooth_eps, l.classes_multipliers, l.cls_normalizer);

						++count;
						++class_count;
						if (iou > 0.5f)
						{
							recall += 1;
						}
						if (iou > 0.75f)
						{
							recall75 += 1;
						}
						avg_iou += iou;
					}
				}
			}
		}

		// averages the deltas obtained by the function: delta_yolo_box()_accumulate
		for (j = 0; j < l.h; ++j)
		{
			for (i = 0; i < l.w; ++i)
			{
				for (n = 0; n < l.n; ++n)
				{
					int box_index = entry_gaussian_index(l, b, n*l.w*l.h + j*l.w + i, 0);
					int class_index = entry_gaussian_index(l, b, n*l.w*l.h + j*l.w + i, 9);
					const int stride = l.w*l.h;

					averages_gaussian_yolo_deltas(class_index, box_index, stride, l.classes, l.delta);
				}
			}
		}
	}


	// calculate: Classification-loss, IoU-loss and Uncertainty-loss
	const int stride = l.w*l.h;
	float* classification_lost = (float *)calloc(l.batch * l.outputs, sizeof(float));
	memcpy(classification_lost, l.delta, l.batch * l.outputs * sizeof(float));

	for (b = 0; b < l.batch; ++b)
	{
		for (j = 0; j < l.h; ++j)
		{
			for (i = 0; i < l.w; ++i)
			{
				for (n = 0; n < l.n; ++n)
				{
					int box_index = entry_gaussian_index(l, b, n*l.w*l.h + j*l.w + i, 0);

					classification_lost[box_index + 0 * stride] = 0;
					classification_lost[box_index + 1 * stride] = 0;
					classification_lost[box_index + 2 * stride] = 0;
					classification_lost[box_index + 3 * stride] = 0;
					classification_lost[box_index + 4 * stride] = 0;
					classification_lost[box_index + 5 * stride] = 0;
					classification_lost[box_index + 6 * stride] = 0;
					classification_lost[box_index + 7 * stride] = 0;
				}
			}
		}
	}
	float class_loss = pow(mag_array(classification_lost, l.outputs * l.batch), 2);
	free(classification_lost);


	float* except_uncertainty_lost = (float *)calloc(l.batch * l.outputs, sizeof(float));
	memcpy(except_uncertainty_lost, l.delta, l.batch * l.outputs * sizeof(float));
	for (b = 0; b < l.batch; ++b)
	{
		for (j = 0; j < l.h; ++j)
		{
			for (i = 0; i < l.w; ++i)
			{
				for (n = 0; n < l.n; ++n)
				{
					int box_index = entry_gaussian_index(l, b, n*l.w*l.h + j*l.w + i, 0);
					except_uncertainty_lost[box_index + 4 * stride] = 0;
					except_uncertainty_lost[box_index + 5 * stride] = 0;
					except_uncertainty_lost[box_index + 6 * stride] = 0;
					except_uncertainty_lost[box_index + 7 * stride] = 0;
				}
			}
		}
	}
	float except_uc_loss = pow(mag_array(except_uncertainty_lost, l.outputs * l.batch), 2);
	free(except_uncertainty_lost);

	*(l.cost) = pow(mag_array(l.delta, l.outputs * l.batch), 2);

	float loss = pow(mag_array(l.delta, l.outputs * l.batch), 2);
	float uc_loss = loss - except_uc_loss;
	float iou_loss = except_uc_loss - class_loss;

	loss /= l.batch;
	class_loss /= l.batch;
	uc_loss /= l.batch;
	iou_loss /= l.batch;

	*cfg_and_state.output
		<< "Region "		<< state.index
		<< ": avg IOU="		<< avg_iou/count
		<< ", class="		<< avg_cat/class_count
		<< ", obj="			<< avg_obj/count
		<< ", no obj="		<< avg_anyobj/(l.w*l.h*l.n*l.batch)
		<< ", recall 50%="	<< recall/count
		<< ", recall 75%="	<< recall75/count
		<< ", count="		<< count
		<< ", class loss="	<< class_loss
		<< ", IOU loss="	<< iou_loss
		<< ", uc loss="		<< uc_loss
		<< ", total loss="	<< loss
		<< std::endl;
}


void backward_gaussian_yolo_layer(Darknet::Layer & l, Darknet::NetworkState state)
{
	TAT(TATPARMS);

	axpy_cpu(l.batch*l.inputs, 1, l.delta, 1, state.delta, 1);
}

void correct_gaussian_yolo_boxes(Darknet::Detection * dets, int n, int w, int h, int netw, int neth, int relative, int letter)
{
	TAT(TATPARMS);

	int i;
	int new_w=0;
	int new_h=0;
	if (letter)
	{
		if (((float)netw / w) < ((float)neth / h))
		{
			new_w = netw;
			new_h = (h * netw) / w;
		}
		else
		{
			new_h = neth;
			new_w = (w * neth) / h;
		}
	}
	else
	{
		new_w = netw;
		new_h = neth;
	}

	for (i = 0; i < n; ++i)
	{
		Darknet::Box b = dets[i].bbox;
		b.x =  (b.x - (netw - new_w)/2./netw) / ((float)new_w/netw);
		b.y =  (b.y - (neth - new_h)/2./neth) / ((float)new_h/neth);
		b.w *= (float)netw/new_w;
		b.h *= (float)neth/new_h;
		if(!relative)
		{
			b.x *= w;
			b.w *= w;
			b.y *= h;
			b.h *= h;
		}
		dets[i].bbox = b;
	}
}


int gaussian_yolo_num_detections(const Darknet::Layer & l, float thresh)
{
	TAT(TATPARMS);

	int i, n;
	int count = 0;
	for (i = 0; i < l.w*l.h; ++i)
	{
		for(n = 0; n < l.n; ++n)
		{
			int obj_index  = entry_gaussian_index(l, 0, n*l.w*l.h + i, 8);
			if(l.output[obj_index] > thresh)
			{
				++count;
			}
		}
	}
	return count;
}


int get_gaussian_yolo_detections(const Darknet::Layer & l, int w, int h, int netw, int neth, float thresh, int *map, int relative, Darknet::Detection * dets, int letter)
{
	TAT(TATPARMS);

	int i,j,n;
	float *predictions = l.output;
	int count = 0;
	for (i = 0; i < l.w*l.h; ++i)
	{
		int row = i / l.w;
		int col = i % l.w;
		for (n = 0; n < l.n; ++n)
		{
			int obj_index  = entry_gaussian_index(l, 0, n*l.w*l.h + i, 8);
			float objectness = predictions[obj_index];
			if (objectness <= thresh)
			{
				continue;    // incorrect behavior for Nan values
			}

			if (objectness > thresh)
			{
				int box_index = entry_gaussian_index(l, 0, n*l.w*l.h + i, 0);
				dets[count].bbox = get_gaussian_yolo_box(predictions, l.biases, l.mask[n], box_index, col, row, l.w, l.h, netw, neth, l.w*l.h, l.yolo_point);
				dets[count].objectness = objectness;
				dets[count].classes = l.classes;

				dets[count].uc[0] = predictions[entry_gaussian_index(l, 0, n*l.w*l.h + i, 1)]; // tx uncertainty
				dets[count].uc[1] = predictions[entry_gaussian_index(l, 0, n*l.w*l.h + i, 3)]; // ty uncertainty
				dets[count].uc[2] = predictions[entry_gaussian_index(l, 0, n*l.w*l.h + i, 5)]; // tw uncertainty
				dets[count].uc[3] = predictions[entry_gaussian_index(l, 0, n*l.w*l.h + i, 7)]; // th uncertainty

				dets[count].points = l.yolo_point;
				//if (l.yolo_point != YOLO_CENTER) dets[count].objectness = objectness = 0;

				for (j = 0; j < l.classes; ++j)
				{
					int class_index = entry_gaussian_index(l, 0, n*l.w*l.h + i, 9 + j);
					float uc_aver = (dets[count].uc[0] + dets[count].uc[1] + dets[count].uc[2] + dets[count].uc[3]) / 4.0;
					float prob = objectness*predictions[class_index] * (1.0 - uc_aver);
					dets[count].prob[j] = (prob > thresh) ? prob : 0;
				}
				++count;
			}
		}
	}
	correct_gaussian_yolo_boxes(dets, count, w, h, netw, neth, relative, letter);
	return count;
}

#ifdef DARKNET_GPU

void forward_gaussian_yolo_layer_gpu(Darknet::Layer & l, Darknet::NetworkState state)
{
	TAT(TATPARMS);

	copy_ongpu(l.batch*l.inputs, state.input, 1, l.output_gpu, 1);
	int b, n;
	for (b = 0; b < l.batch; ++b)
	{
		for(n = 0; n < l.n; ++n)
		{
			// x : mu, sigma
			int index = entry_gaussian_index(l, b, n*l.w*l.h, 0);
			activate_array_ongpu(l.output_gpu + index, 2*l.w*l.h, LOGISTIC);
			scal_add_ongpu(l.w*l.h, l.scale_x_y, -0.5*(l.scale_x_y - 1), l.output_gpu + index, 1);      // scale x
			// y : mu, sigma
			index = entry_gaussian_index(l, b, n*l.w*l.h, 2);
			activate_array_ongpu(l.output_gpu + index, 2*l.w*l.h, LOGISTIC);
			scal_add_ongpu(l.w*l.h, l.scale_x_y, -0.5*(l.scale_x_y - 1), l.output_gpu + index, 1);      // scale y
			// w : sigma
			index = entry_gaussian_index(l, b, n*l.w*l.h, 5);
			activate_array_ongpu(l.output_gpu + index, l.w*l.h, LOGISTIC);
			// h : sigma
			index = entry_gaussian_index(l, b, n*l.w*l.h, 7);
			activate_array_ongpu(l.output_gpu + index, l.w*l.h, LOGISTIC);
			// objectness & class
			index = entry_gaussian_index(l, b, n*l.w*l.h, 8);
			activate_array_ongpu(l.output_gpu + index, (1+l.classes)*l.w*l.h, LOGISTIC);
		}
	}

	if (!state.train || l.onlyforward)
	{
		//cuda_pull_array(l.output_gpu, l.output, l.batch*l.outputs);
		cuda_pull_array_async(l.output_gpu, l.output, l.batch*l.outputs);
		CHECK_CUDA(cudaPeekAtLastError());
		return;
	}

	float *in_cpu = (float *)calloc(l.batch*l.inputs, sizeof(float));
	cuda_pull_array(l.output_gpu, l.output, l.batch*l.outputs);
	memcpy(in_cpu, l.output, l.batch*l.outputs * sizeof(float));
	float *truth_cpu = 0;
	if (state.truth)
	{
		int num_truth = l.batch*l.truths;
		truth_cpu = (float *)calloc(num_truth, sizeof(float));
		cuda_pull_array(state.truth, truth_cpu, num_truth);
	}
	Darknet::NetworkState cpu_state = state;
//	cpu_state.net = state.net;
//	cpu_state.index = state.index;
//	cpu_state.train = state.train;
	cpu_state.truth = truth_cpu;
	cpu_state.input = in_cpu;
	forward_gaussian_yolo_layer(l, cpu_state);
	//forward_yolo_layer(l, state);
	cuda_push_array(l.delta_gpu, l.delta, l.batch*l.outputs);
	free(in_cpu);
	if (cpu_state.truth)
	{
		free(cpu_state.truth);
	}
}

void backward_gaussian_yolo_layer_gpu(Darknet::Layer & l, Darknet::NetworkState state)
{
	TAT(TATPARMS);

	axpy_ongpu(l.batch*l.inputs, l.delta_normalizer, l.delta_gpu, 1, state.delta, 1);
}
#endif

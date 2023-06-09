
#pragma once
#include "utility.hpp"

namespace PaddleOCR
{

    cv::Mat GetRotateCropImage(const cv::Mat &srcimage, std::vector<std::vector<int>> box)
    {
        cv::Mat image;
        srcimage.copyTo(image);
        std::vector<std::vector<int>> points = box;

        int x_collect[4] = {box[0][0], box[1][0], box[2][0], box[3][0]};
        int y_collect[4] = {box[0][1], box[1][1], box[2][1], box[3][1]};
        int left = int(*std::min_element(x_collect, x_collect + 4));
        int right = int(*std::max_element(x_collect, x_collect + 4));
        int top = int(*std::min_element(y_collect, y_collect + 4));
        int bottom = int(*std::max_element(y_collect, y_collect + 4));

        cv::Mat img_crop;
        image(cv::Rect(left, top, right - left, bottom - top)).copyTo(img_crop);

        for (int i = 0; i < points.size(); i++)
        {
            points[i][0] -= left;
            points[i][1] -= top;
        }

        int img_crop_width = int(sqrt(pow(points[0][0] - points[1][0], 2) +
                                      pow(points[0][1] - points[1][1], 2)));
        int img_crop_height = int(sqrt(pow(points[0][0] - points[3][0], 2) +
                                       pow(points[0][1] - points[3][1], 2)));

        cv::Point2f pts_std[4];
        pts_std[0] = cv::Point2f(0., 0.);
        pts_std[1] = cv::Point2f(img_crop_width, 0.);
        pts_std[2] = cv::Point2f(img_crop_width, img_crop_height);
        pts_std[3] = cv::Point2f(0.f, img_crop_height);

        cv::Point2f pointsf[4];
        pointsf[0] = cv::Point2f(points[0][0], points[0][1]);
        pointsf[1] = cv::Point2f(points[1][0], points[1][1]);
        pointsf[2] = cv::Point2f(points[2][0], points[2][1]);
        pointsf[3] = cv::Point2f(points[3][0], points[3][1]);

        cv::Mat M = cv::getPerspectiveTransform(pointsf, pts_std);

        cv::Mat dst_img;
        cv::warpPerspective(img_crop, dst_img, M,
                            cv::Size(img_crop_width, img_crop_height),
                            cv::BORDER_REPLICATE);

        if (float(dst_img.rows) >= float(dst_img.cols) * 1.5)
        {
            cv::Mat srcCopy = cv::Mat(dst_img.rows, dst_img.cols, dst_img.depth());
            cv::transpose(dst_img, srcCopy);
            cv::flip(srcCopy, srcCopy, 0);
            return srcCopy;
        }
        else
        {
            return dst_img;
        }
    }

    cv::Mat resizeImageExt(const cv::Mat &mat, int width, int height, RESIZE_MODE resizeMode,
                           cv::InterpolationFlags interpolationMode, cv::Rect *roi, cv::Scalar BorderConstant)
    {
        if (width == mat.cols && height == mat.rows)
        {
            return mat;
        }

        cv::Mat dst;

        switch (resizeMode)
        {
        case RESIZE_FILL:
        {
            cv::resize(mat, dst, cv::Size(width, height), interpolationMode);
            if (roi)
            {
                *roi = cv::Rect(0, 0, width, height);
            }
            break;
        }
        case RESIZE_KEEP_ASPECT:
        case RESIZE_KEEP_ASPECT_LETTERBOX:
        {
            double scale = std::min(static_cast<double>(width) / mat.cols, static_cast<double>(height) / mat.rows);
            cv::Mat resizedImage;
            cv::resize(mat, resizedImage, cv::Size(0, 0), scale, scale, interpolationMode);

            int dx = resizeMode == RESIZE_KEEP_ASPECT ? 0 : (width - resizedImage.cols) / 2;
            int dy = resizeMode == RESIZE_KEEP_ASPECT ? 0 : (height - resizedImage.rows) / 2;

            cv::copyMakeBorder(resizedImage, dst, dy, height - resizedImage.rows - dy,
                               dx, width - resizedImage.cols - dx, cv::BORDER_CONSTANT, BorderConstant);
            if (roi)
            {
                *roi = cv::Rect(dx, dy, resizedImage.cols, resizedImage.rows);
            }
            break;
        }
        }
        return dst;
    }

    void normalize(cv::Mat *im, const std::vector<float> &mean, const std::vector<float> &scale, const bool is_scale)
    {
        double e = 1.0;
        if (is_scale)
        {
            e /= 255.0;
        }
        (*im).convertTo(*im, CV_32FC3, e);
        std::vector<cv::Mat> bgr_channels(3);
        cv::split(*im, bgr_channels);
        for (auto i = 0; i < bgr_channels.size(); i++)
        {
            bgr_channels[i].convertTo(bgr_channels[i], CV_32FC1, 1.0 * scale[i],
                                      (0.0 - mean[i]) * scale[i]);
        }
        cv::merge(bgr_channels, *im);
    }

    // up to down, left to right
    static bool comparison_box(const OCRPredictResult &result1, const OCRPredictResult &result2)
    {
        if (result1.box[0][1] < result2.box[0][1])
        {
            return true;
        }
        else if (result1.box[0][1] == result2.box[0][1])
        {
            return result1.box[0][0] < result2.box[0][0];
        }
        else
        {
            return false;
        }
    }

    void sorted_boxes(std::vector<OCRPredictResult> &ocr_result)
    {
        std::sort(ocr_result.begin(), ocr_result.end(), comparison_box);
        if (ocr_result.size() > 0)
        {
            for (int i = 0; i < ocr_result.size() - 1; i++)
            {
                for (int j = i; j > 0; j--)
                {
                    if (abs(ocr_result[j + 1].box[0][1] - ocr_result[j].box[0][1]) < 10 &&
                        (ocr_result[j + 1].box[0][0] < ocr_result[j].box[0][0]))
                    {
                        std::swap(ocr_result[i], ocr_result[i + 1]);
                    }
                }
            }
        }
    }

    void print_result(const std::vector<OCRPredictResult> &ocr_result)
    {
        for (int i = 0; i < ocr_result.size(); i++)
        {
            std::cout << i << "\t";
            // det
            std::vector<std::vector<int>> boxes = ocr_result[i].box;
            if (boxes.size() > 0)
            {
                std::cout << "det boxes: [";
                for (int n = 0; n < boxes.size(); n++)
                {
                    std::cout << '[' << boxes[n][0] << ',' << boxes[n][1] << "]";
                    if (n != boxes.size() - 1)
                    {
                        std::cout << ',';
                    }
                }
                std::cout << "] ";
            }
            // rec
            if (ocr_result[i].score != -1.0)
            {
                std::cout << "rec text: " << ocr_result[i].text
                          << " rec score: " << ocr_result[i].score << " ";
            }

            // cls
            if (ocr_result[i].cls_label != -1)
            {
                std::cout << "cls label: " << ocr_result[i].cls_label
                          << " cls score: " << ocr_result[i].cls_score;
            }
            std::cout << std::endl;
        }
    }

    void VisualizeBboxes(const cv::Mat &srcimg, const std::vector<OCRPredictResult> &ocr_result, const std::string &save_path)
    {
        cv::Mat img_vis;
        srcimg.copyTo(img_vis);
        for (int n = 0; n < ocr_result.size(); n++)
        {
            cv::Point rook_points[4];
            for (int m = 0; m < ocr_result[n].box.size(); m++)
            {
                rook_points[m] =
                    cv::Point(int(ocr_result[n].box[m][0]), int(ocr_result[n].box[m][1]));
            }

            const cv::Point *ppt[1] = {rook_points};
            int npt[] = {4};
            cv::polylines(img_vis, ppt, npt, 1, 1, CV_RGB(0, 255, 0), 2, 8, 0);

            cv::Point text_position = rook_points[3];
            int fontface = cv::FONT_HERSHEY_SIMPLEX;
            double scale = 0.7;
            int thickness = 2;
            cv::putText(img_vis, ocr_result[n].text, text_position, fontface, scale, CV_RGB(255, 0, 0), thickness, 6);
        }

        cv::imwrite(save_path, img_vis);
        std::cout << "The detection visualized image saved in " + save_path << std::endl;
        cv::imshow("OCR Result", img_vis);
        int key = cv::waitKey(1);
    }

} // namespace PaddleOCR
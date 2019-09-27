//include librealsense2 library
#include <sys/stat.h>
#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>

#include <fstream>
#include <stdio.h>
#include <stdlib.h>

//include opencv library
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core.hpp>

//include  some library for data-structure
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <queue>
#include <unordered_set>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <Windows.h>
#include <experimental/filesystem>

using namespace std;
using namespace cv;
namespace fs = std::experimental::filesystem;

////////////////////////////////////////////////////////////////////

using pixel = pair<float, float>;
pixel u, v, u1, v1, z, z1, q, q1, object_center;


float dist_3d1(const rs2::depth_frame& frame, pixel u, pixel v, int w, int h, int wid, int hei);
float dist_3d2(const rs2::depth_frame& frame, pixel u, pixel v);
float dist_3d3(const rs2::depth_frame& frame, pixel u, pixel v, float dist_to_object);
vector<Point2f> getCorner(rs2::depth_frame depth, int width, int height);

vector<Point2f> points;
Mat img, img1;
int m = 0;

///////////////////////////////////// click button function for manual mode /////////////////////////////////////
void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		if (m == 0)
		{
			u1.first = x;
			u1.second = y;

			u.first = u1.first;
			u.second = u1.second;
		}
		if (m == 1)
		{
			v1.first = x;
			v1.second = y;

			v.first = v1.first;
			v.second = v1.second;
		}
		if (m == 2)
		{
			z1.first = x;
			z1.second = y;

			z.first = z1.first;
			z.second = z1.second;
		}
		if (m == 3)
		{
			q1.first = x;
			q1.second = y;

			q.first = q1.first;
			q.second = q1.second;
		}
		m++;
	}

}
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////// check if file is existed!!! ///////////////////////////////
bool FileExists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}
/////////////////////////////////////////////////////////////////////////////////

int stt2;
int main(int argc, char * argv[]) try
{
	///////////////////////// path of file ///////////////////
	string path = fs::current_path().generic_string(); ////////// read the current path of our project
	string filename1 = path + "/control/start/start";
	string filename2 = path + "/control/close/close";
	string filename3 = path + "/number.txt";
	string filename4 = filename1;
	string filename5 = filename2;
	string filename6 = path + "/control/state/state";
	string filename7 = filename6;
	string filename8 = path + "/control/manual/manual";
	string filename9 = path + "/control/auto/auto";
	string filename10 = filename8;
	string filename11 = filename9;
	string filename12 = path + "/control/image/image";
	string filename13 = filename12;

	//////////////////////////////////////////////////////////

	int n = 0;
	int n1 = 0;
	int n2 = 0;
	int n3 = 0;

	string stt1;

	ifstream file2("number.txt");
	if (file2.is_open())
	{
		getline(file2, stt1);
		file2.close();
		stt2 = stoi(stt1);

	}
	cout << stt2 << endl;
	const char x = '/';
	const char y = '\\';
	replace(filename4.begin(), filename4.end(), x, y);
	replace(filename5.begin(), filename5.end(), x, y);
	replace(filename7.begin(), filename7.end(), x, y);
	replace(filename10.begin(), filename10.end(), x, y);
	replace(filename11.begin(), filename11.end(), x, y);
	replace(filename13.begin(), filename13.end(), x, y);
	replace(path.begin(), path.end(), x, y);

	/////////////////////// when the app open, delete all the file have been created from the last open //////////////////
	for (int i = 0; i < (stt2 + 1); i++)
	{
		string remove_path1 = filename4 + to_string(i) + ".txt";
		string remove_path2 = filename5 + to_string(i) + ".txt";
		string remove_path3 = filename7 + to_string(i) + ".txt";
		string remove_path4 = filename10 + to_string(i) + ".txt";
		string remove_path5 = filename11 + to_string(i) + ".txt";
		string remove_path6 = filename13 + to_string(i) + ".tif";

		const char * str1 = remove_path1.c_str();
		const char * str2 = remove_path2.c_str();
		const char * str3 = remove_path3.c_str();
		const char * str4 = remove_path4.c_str();
		const char * str5 = remove_path5.c_str();
		const char * str6 = remove_path6.c_str();

		remove(str1);
		remove(str2);
		remove(str3);
		remove(str4);
		remove(str5);
		remove(str6);
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////// Defining all the processing block we going to use /////////////////////
	rs2::hole_filling_filter hole_filter;
	hole_filter.set_option(RS2_OPTION_HOLES_FILL, 1);
	rs2::colorizer color_map;
	// Use black to white color map
	color_map.set_option(RS2_OPTION_COLOR_SCHEME, 2.f);
	// Decimation filter reduces the amount of data (while preserving best samples)
	rs2::decimation_filter dec;
	// If the demo is too slow, make sure you run in Release (-DCMAKE_BUILD_TYPE=Release)
	// but you can also increase the following parameter to decimate depth more (reducing quality)
	dec.set_option(RS2_OPTION_FILTER_MAGNITUDE, 2);
	// Define transformations from and to Disparity domain
	rs2::disparity_transform depth2disparity;
	rs2::disparity_transform disparity2depth(false);
	// Define spatial filter (edge-preserving)
	rs2::spatial_filter spat;
	// Enable hole-filling
	// Hole filling is an agressive heuristic and it gets the depth wrong many times
	// However, this demo is not built to handle holes
	// (the shortest-path will always prefer to "cut" through the holes since they have zero 3D distance)
	spat.set_option(RS2_OPTION_HOLES_FILL, 1); // 5 = fill all the zero pixels
	// Define temporal filter
	rs2::temporal_filter temp;
	// Spatially align all streams to depth viewport
	// We do this because:
	//   a. Usually depth has wider FOV, and we only really need depth for this demo
	//   b. We don't want to introduce new holes
	rs2::align align_to(RS2_STREAM_COLOR);
	rs2::align align_to1(RS2_STREAM_COLOR);
	rs2::pipeline p;
	rs2::config cfg;
	cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 0);
	cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 0);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	auto profile = p.start(cfg);
	auto sensor = profile.get_device().first<rs2::depth_sensor>();

	//set the device to high accuraccy preset of the d400
	if (sensor && sensor.is<rs2::depth_stereo_sensor>())
	{
		sensor.set_option(RS2_OPTION_VISUAL_PRESET, RS2_RS400_VISUAL_PRESET_DEFAULT);
	}

	auto stream = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
	rs2::frame_queue postprocessed_frames, postprocessed_frames1;

	std::atomic_bool alive{ true };
	int w;
	int h;

	///////////////////////// run processing block on background thread ////////////////////////////////
	std::thread video_processing_thread([&]() {
		while (alive)
		{
			// Fetch frames from the pipeline and send them for processing
			rs2::frameset data, data1;
			if (p.poll_for_frames(&data))
			{
				data1 = data;
				data1 = data1.apply_filter(align_to1);
				postprocessed_frames1.enqueue(data1);

				// Decimation will reduce the resultion of the depth image,
				// closing small holes and speeding-up the algorithm
				data = data.apply_filter(dec);
				// To make sure far-away objects are filtered y
				// we try to switch to disparity domain
				data = data.apply_filter(depth2disparity);
				// Apply spatial filtering
				data = data.apply_filter(spat);
				// Apply temporal filtering
				data = data.apply_filter(temp);
				// If we are in disparity domain, switch back to depth
				data = data.apply_filter(disparity2depth);
				//Apply hole filtering
				data = data.apply_filter(hole_filter);
				data = data.apply_filter(align_to);
				//// Apply color map for visualization of depth
				data = data.apply_filter(color_map);
				// Send resulting frames for visualization in the main thread
				auto fr = data.get_color_frame();
				//auto colorized_depth = data.first(RS2_STREAM_DEPTH, RS2_FORMAT_RGB8);
				w = fr.as<rs2::video_frame>().get_width();
				h = fr.as<rs2::video_frame>().get_height();


				Mat image(Size(w, h), CV_8UC3, (void*)fr.get_data(), Mat::AUTO_STEP);
				img = image;
				circle(image, Point(u1.first, u1.second), 2, Scalar(255, 0, 0), CV_FILLED, 3, 0);
				circle(image, Point(v1.first, v1.second), 2, Scalar(0, 255, 0), CV_FILLED, 3, 0);
				circle(image, Point(z1.first, z1.second), 2, Scalar(0, 0, 255), CV_FILLED, 3, 0);
				circle(image, Point(q1.first, q1.second), 2, Scalar(0, 255, 255), CV_FILLED, 3, 0);

				postprocessed_frames.enqueue(data);

			}
		}
	});
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////// main thread //////////////////////////////////////////////////////
	rs2::frameset current_frameset, current_frameset1;
	while (true)
	{

		string start_path = filename1 + to_string(n) + ".txt";
		//string close_path = filename2 + to_string(n) + ".txt";


		while (FileExists(start_path) == 0)
		{
			//cout << "stop!!!" << endl;
			Sleep(5);
		}


		while (FileExists(start_path) == 1)
		{


			string close_path = filename2 + to_string(n) + ".txt";
			postprocessed_frames.poll_for_frame(&current_frameset);
			//int s = 0;
			//cout << "new frame" << endl;
			if (current_frameset)
			{

				//////////////////////////////// MANUAL MODE ///////////////////////////////////
				string manual_path = filename8 + to_string(n2) + ".txt";
				string auto_path = filename9 + to_string(n3) + ".txt";
				//cout << "start!!!" << endl;

				while (FileExists(manual_path) == 1)
				{
					const auto window_name = "img";
					namedWindow(window_name, WINDOW_AUTOSIZE);

					setMouseCallback(window_name, CallBackFunc, NULL);
					if (m == 4) { m = 0; }
					imshow(window_name, img);

					auto depth = current_frameset.get_depth_frame();
					int wid = depth.get_width();
					int hei = depth.get_height();

					string state_path = filename6 + to_string(n1) + ".txt";
					string image_path = filename12 + to_string(n1) + ".tif";
					while (FileExists(state_path) == 1)
					{
						line(img, Point(u.first, u.second), Point(v.first, v.second), (0, 0, 255), 2);
						line(img, Point(v.first, v.second), Point(z.first, z.second), (0, 0, 255), 2);
						resize(img, img, Size(480, 360));
						imwrite(image_path, img);
						Sleep(1000);
						//cout << "Sleep" << endl;
						n1++;
						state_path = filename6 + to_string(n1) + ".txt";


					}


					float air_dist1_1 = dist_3d1(depth, u, v, w, h, wid, hei);
					float air_dist2_1 = dist_3d1(depth, v, z, w, h, wid, hei);
					//float air_dist3_1 = dist_3d1(depth, z, q, w, h, wid, hei);
					//float air_dist4_1 = dist_3d1(depth, q, u, w, h, wid, hei);

					object_center.first = (u.first + v.first + z.first + q.first) / 4;
					object_center.second = (u.second + v.second + z.second + q.second) / 4;
					float img_dis1 = depth.get_distance(object_center.first / 2, object_center.second / 2);
					float img_dis2 = depth.get_distance((object_center.first + w) / 2, (object_center.second) / 2);
					float img_dis3 = depth.get_distance((object_center.first + w) / 2, (object_center.second + h) / 2);
					float img_dis4 = depth.get_distance(object_center.first / 2, (object_center.second + h) / 2);
					float object_height = (img_dis1 + img_dis2 + img_dis3 + img_dis4) / 4 - depth.get_distance(object_center.first, object_center.second);
					object_height = object_height * 100;
					string dist1 = to_string(air_dist1_1 * 100);
					string dist2 = to_string(air_dist2_1 * 100);
					string a = path + "/distance1.txt";
					ofstream myfile1(a);
					if (myfile1.is_open())
					{
						myfile1 << dist1;
						//cout << "ghi xong" << endl;
						myfile1.close();
					}

					string b = path + "/distance2.txt";
					ofstream myfile2(b);
					if (myfile2.is_open())
					{
						myfile2 << dist2;
						myfile2.close();
					}

					string c = path + "/distance3.txt";
					ofstream myfile3(c);
					if (myfile3.is_open())
					{
						myfile3 << to_string(object_height);
					}
					myfile3.close();


					pixel u1, v1;
					if (FileExists(close_path) == 1)
					{
						n++;
						string number;
						if (n < n1) { number = to_string(n1); }
						else { number = to_string(n); }
						ofstream myfile1("number.txt");
						if (myfile1.is_open())
						{
							myfile1 << number;
							myfile1.close();
						}
						destroyAllWindows();
						n2++;



						goto loop;

					}
					char k = (char)waitKey(1);
					if (k == 27)
						break;

					Sleep(5);
				}
				///////////////////////////////////////////////////////////////////////////////

				///////////////////////////// AUTO MODE /////////////////////////////////////

				while (FileExists(auto_path) == 1)
				{
					const auto window_name = "img";
					namedWindow(window_name, WINDOW_AUTOSIZE);
					imshow(window_name, img);
					postprocessed_frames1.poll_for_frame(&current_frameset1);
					auto depth = current_frameset1.get_depth_frame();
					int wid = depth.get_width();
					int hei = depth.get_height();

					vector<Point2f> pts;
					pts = getCorner(depth, wid, hei);
					u.first = pts[0].x;
					u.second = pts[0].y;
					v.first = pts[1].x;
					v.second = pts[1].y;
					z.first = pts[2].x;
					z.second = pts[2].y;
					q.first = pts[3].x;
					q.second = pts[3].y;
					float dist_to_object = pts[4].y;
					float object_height = pts[4].x;



					string dist3, dist4;

					string state_path = filename6 + to_string(n1) + ".txt";
					string image_path = filename12 + to_string(n1) + ".tif";
					while (FileExists(state_path) == 1)
					{
						line(img, Point(u.first, u.second), Point(v.first, v.second), (0, 0, 255), 2);
						line(img, Point(v.first, v.second), Point(z.first, z.second), (0, 0, 255), 2);
						resize(img, img, Size(480, 360));
						imwrite(image_path, img);
						Sleep(1000);
						//cout << "Sleep" << endl;
						n1++;
						state_path = filename6 + to_string(n1) + ".txt";

					}


					float air_dist3 = dist_3d3(depth, u, v, dist_to_object);
					float air_dist4 = dist_3d3(depth, v, z, dist_to_object);
					float air_dist5 = dist_3d3(depth, z, q, dist_to_object);
					float air_dist6 = dist_3d3(depth, q, u, dist_to_object);

					air_dist3 = (air_dist3 + air_dist5) / 2;
					air_dist4 = (air_dist4 + air_dist6) / 2;


					dist3 = to_string((air_dist3) * 100);
					dist4 = to_string((air_dist4) * 100);




					string a = path + "/distance1.txt";
					ofstream myfile1(a);
					if (myfile1.is_open())
					{
						myfile1 << dist3;
					}
					myfile1.close();

					string b = path + "/distance2.txt";
					ofstream myfile2(b);
					if (myfile2.is_open())
					{
						myfile2 << dist4;
					}
					myfile2.close();

					string c = path + "/distance3.txt";
					ofstream myfile3(c);
					if (myfile3.is_open())
					{
						myfile3 << to_string(object_height);
					}
					myfile3.close();



					pixel u1, v1;


					Sleep(5);
					if (FileExists(close_path) == 1)
					{
						n++;
						string number;
						if (n < n1) { number = to_string(n1); }
						else { number = to_string(n); }
						ofstream myfile1("number.txt");
						if (myfile1.is_open())
						{
							myfile1 << number;
							myfile1.close();
						}
						destroyAllWindows();
						n3++;
						goto loop;

					}
					char k = (char)waitKey(1);
					if (k == 27)
						break;

				}
				//////////////////////////////////////////////////////////////////////////
			loop:
				Sleep(5);


			}

		}


	}

	alive = false;
	video_processing_thread.join();

	return EXIT_SUCCESS;
	destroyAllWindows();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

catch (const rs2::error & e)
{
	std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
	return EXIT_FAILURE;
}
catch (const std::exception& e)
{
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}


///////////////////////////// converting between pixels and points in 3D and Calculate the distance //////////////////////////////
float dist_3d1(const rs2::depth_frame& frame, pixel u, pixel v, int w, int h, int wid, int hei)
{
	float upixel[2];
	float upoint[3];

	float vpixel[2];
	float vpoint[3];

	upixel[0] = (u.first / w)*wid;
	upixel[1] = (u.second / h)*hei;
	vpixel[0] = (v.first / w)*wid;
	vpixel[1] = (v.second / h)*hei;

	auto udist = frame.get_distance(upixel[0], upixel[1]);
	auto vdist = frame.get_distance(vpixel[0], vpixel[1]);

	rs2_intrinsics intr = frame.get_profile().as<rs2::video_stream_profile>().get_intrinsics();
	rs2_deproject_pixel_to_point(upoint, &intr, upixel, udist);
	rs2_deproject_pixel_to_point(vpoint, &intr, vpixel, vdist);

	float distance = sqrt(pow(upoint[0] - vpoint[0], 2) +
		pow(upoint[1] - vpoint[1], 2) +
		pow(upoint[2] - vpoint[2], 2));

	return distance;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////// converting between pixels and points in 3D and Calculate the distance //////////////////////////////
float dist_3d2(const rs2::depth_frame& frame, pixel u, pixel v)
{
	float upixel[2];
	float upoint[3];

	float vpixel[2];
	float vpoint[3];

	upixel[0] = u.first;
	upixel[1] = u.second;
	vpixel[0] = v.first;
	vpixel[1] = v.second;

	auto udist = frame.get_distance(upixel[0], upixel[1]);
	auto vdist = frame.get_distance(vpixel[0], vpixel[1]);

	rs2_intrinsics intr = frame.get_profile().as<rs2::video_stream_profile>().get_intrinsics();
	rs2_deproject_pixel_to_point(upoint, &intr, upixel, udist);
	rs2_deproject_pixel_to_point(vpoint, &intr, vpixel, vdist);

	float distance = sqrt(pow(upoint[0] - vpoint[0], 2) +
		pow(upoint[1] - vpoint[1], 2) +
		pow(upoint[2] - vpoint[2], 2));

	return distance;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////// converting between pixels and points in 3D and Calculate the distance //////////////////////////////
float dist_3d3(const rs2::depth_frame& frame, pixel u, pixel v, float dist_to_object)
{
	float upixel[2];
	float upoint[3];

	float vpixel[2];
	float vpoint[3];

	upixel[0] = u.first;
	upixel[1] = u.second;
	vpixel[0] = v.first;
	vpixel[1] = v.second;

	auto udist = dist_to_object;
	auto vdist = dist_to_object;

	rs2_intrinsics intr = frame.get_profile().as<rs2::video_stream_profile>().get_intrinsics();
	rs2_deproject_pixel_to_point(upoint, &intr, upixel, udist);
	rs2_deproject_pixel_to_point(vpoint, &intr, vpixel, vdist);

	float distance = sqrt(pow(upoint[0] - vpoint[0], 2) +
		pow(upoint[1] - vpoint[1], 2) +
		pow(upoint[2] - vpoint[2], 2));

	return distance;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////// detect corner for auto mode /////////////////////////////////////////

vector<Point2f> getCorner(rs2::depth_frame depth, int width, int height)
{
	//int64 t0 = cv::getTickCount();

	Point2i pointCentral;
	pointCentral.x = width / 2;
	pointCentral.y = height / 2;
#define lengthWidth 25
#define lengthHeight 20
	uint8_t radius = (uint)(width / 2) / lengthWidth;
	float vicinityDistance = 0.025f;
	float setVicinityDistance[2] = { 0.012f, 0.02f };

	Mat frameDepth = Mat::zeros(Size(width, height), CV_8U);
	Mat frametemp = frameDepth.clone();
	vector<Point2f> fourCorner;
	float objectHeight;

	float distanceCenter = depth.get_distance(width / 2, height / 2);
	if (distanceCenter < 0.005f) distanceCenter = depth.get_distance(width / 2 + 7, height / 2);
	if (distanceCenter < 0.005f) distanceCenter = depth.get_distance(width / 2 + 14, height / 2);

	float setDistance[6][2] = { 0.0f };
	float maxDistance = 0.4f;
	float objectDistance = 0.0f;
	float frequencyDistancePoint = 0.0f;
	float sumMaxDistance = 0.0f;
	float sumObjectDistance = 0.0f;
	uint sumObjectPoint = 0;
	uint sumMaxPoint = 0;
	uint numberNext;
	uint8_t numberloop = 0;

	bool noObject = true;
	for (uint16_t i = 5; i < pointCentral.x - 5; i += 5)
	{
		float distance1 = depth.get_distance(pointCentral.x + i, pointCentral.y);
		float distance2 = depth.get_distance(pointCentral.x - i, pointCentral.y);
		if ((distance1 > 0.005f) && (distance1 < 1.5f) && (abs(distance1 - distanceCenter) > vicinityDistance))
		{
			noObject = false;
			if (distance1 > distanceCenter)
			{
				objectDistance = distanceCenter;
				maxDistance = distance1;
			}
			else
			{
				objectDistance = distance1;
				maxDistance = distanceCenter;
			}
			break;
		}
		if ((distance2 > 0.01f) && (distance2 < 1.5f) && (abs(distance2 - distanceCenter) > vicinityDistance))
		{
			noObject = false;
			if (distance2 > distanceCenter)
			{
				objectDistance = distanceCenter;
				maxDistance = distance2;
			}
			else
			{
				objectDistance = distance2;
				maxDistance = distanceCenter;
			}
			break;
		}
	}
	if (noObject)
	{
		for (uint16_t i = 5; i < pointCentral.y - 5; i += 5)
		{
			float distance1 = depth.get_distance(pointCentral.x, pointCentral.y + i);
			float distance2 = depth.get_distance(pointCentral.x, pointCentral.y - i);
			if ((distance1 > 0.005f) && (distance1 < 1.5f) && (abs(distance1 - distanceCenter) > vicinityDistance))
			{
				noObject = false;
				if (distance1 > distanceCenter)
				{
					objectDistance = distanceCenter;
					maxDistance = distance1;
				}
				else
				{
					objectDistance = distance1;
					maxDistance = distanceCenter;
				}
				break;
			}
			if ((distance2 > 0.01f) && (distance2 < 1.5f) && (abs(distance2 - distanceCenter) > vicinityDistance))
			{
				noObject = false;
				if (distance2 > distanceCenter)
				{
					objectDistance = distanceCenter;
					maxDistance = distance2;
				}
				else
				{
					objectDistance = distance2;
					maxDistance = distanceCenter;
				}
				break;
			}
		}
	}

	if (noObject)
	{
		maxDistance = distanceCenter;
		do
		{
			if (numberloop == (sizeof(setVicinityDistance) / sizeof(float)))
			{
				fourCorner.push_back(Point2f(0.0f, 0.0f));
				return fourCorner;
			}
			numberNext = 0;
			vicinityDistance = setVicinityDistance[numberloop];
			for (uint r = 1; r < radius; r++)
			{
				int16_t x = pointCentral.x - (r + 1)*lengthWidth;
				int16_t y = pointCentral.y - r * lengthHeight;
				for (uint numberPointloop = 0; numberPointloop < 8 * r; numberPointloop++)
				{
					if (numberPointloop <= 2 * r)
					{
						x += lengthWidth;
					}
					else if (numberPointloop <= 4 * r)
					{
						y += lengthHeight;
					}
					else if (numberPointloop <= 6 * r)
					{
						x -= lengthWidth;
					}
					else
					{
						y -= lengthHeight;
					}
					frametemp.at<uchar>(y, x) = 255;
					float distance = depth.get_distance(x, y);
					if ((distance > 0.005f) && (distance < 2.0f) && (abs(maxDistance - distance) > 0.025f))
					{
						bool flagNext = true;
						for (uint8_t i = 0; i < numberNext; i++)
						{
							float subDistance = abs(setDistance[i][0] - distance);
							if (subDistance <= vicinityDistance)
							{
								setDistance[i][1]++;
								flagNext = false;
								break;
							}
						}
						if (flagNext && (numberNext < 6))
						{
							setDistance[numberNext][0] = distance;
							numberNext++;
						}
					}
				}
			}
			numberloop++;
			//cout<<"so luong: "<<numberNext<<endl;

		} while (numberNext > 2);
		//imshow("frametemp", frametemp);

		if (numberNext == 0)
		{
			fourCorner.push_back(Point2f(0.0f, 0.0f));
			return fourCorner;
		}

		for (uint16_t i = 0; i < numberNext; i++)
		{
			if ((setDistance[i][1] > frequencyDistancePoint))
			{
				frequencyDistancePoint = setDistance[i][1];
				objectDistance = setDistance[i][0];
			}
		}

	}


	vicinityDistance = 0.02f;
	if ((maxDistance - objectDistance) > 0.13f) vicinityDistance = 0.025f;
	else if ((maxDistance - objectDistance) < 0.7f) vicinityDistance = 0.012f;
	for (uint16_t x = 0; x < width; x++)
	{
		for (uint16_t y = 0; y < height; y++)
		{
			float distance = depth.get_distance(x, y);
			if (abs(distance - objectDistance) <= vicinityDistance)
			{
				frameDepth.at<uchar>(y, x) = 255;
				sumObjectDistance += distance;
				sumObjectPoint++;
			}
			else if (abs(distance - maxDistance) <= vicinityDistance)
			{
				sumMaxDistance += distance;
				sumMaxPoint++;
			}
		}
	}

	//-------------Result Object Height--------------------//
	objectHeight = (sumMaxDistance / (float)sumMaxPoint - sumObjectDistance / (float)sumObjectPoint) * 100;
	//cout<<"chieu cao vat the: "<<objectHeight<<endl;
	objectDistance = sumObjectDistance / (float)sumObjectPoint;
	/////////////////////////////////////////////////////////

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	Mat imageContours = frameDepth.clone();
	/// Find contours
	findContours(imageContours, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	int indexmaxContours[2] = { 0, 0 };
	for (uint8_t i = 0; i < contours.size(); i++)
	{
		if (contours[i].size() > indexmaxContours[1])
		{
			indexmaxContours[0] = i;
			indexmaxContours[1] = contours[i].size();
		}
	}

	Point2f fourPoint[4];
	RotatedRect box = minAreaRect(contours[indexmaxContours[0]]);
	box.points(fourPoint);

	//imshow("frameDepth", frameDepth);
	for (uint8_t i = 0; i < 4; i++)
	{
		fourCorner.push_back(fourPoint[i]);
	}
	fourCorner.push_back(Point2f(objectHeight, objectDistance));
	//////////////////////////////////////////////////////////////////

	return fourCorner;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
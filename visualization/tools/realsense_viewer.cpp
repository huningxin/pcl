/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2014-, Open Perception, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder(s) nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <iostream>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>

#include <pcl/io/pcd_io.h>
#include <pcl/common/time.h>
#include <pcl/console/print.h>
#include <pcl/console/parse.h>
#include <pcl/io/io_exception.h>
#include <pcl/io/realsense_grabber.h>
#include <pcl/visualization/pcl_visualizer.h> 

using namespace pcl::console;

void
printHelp (int, char **argv)
{
  std::cout << std::endl;
  std::cout << "****************************************************************************" << std::endl;
  std::cout << "*                                                                          *" << std::endl;
  std::cout << "*                       REALSENSE VIEWER - Usage Guide                     *" << std::endl;
  std::cout << "*                                                                          *" << std::endl;
  std::cout << "****************************************************************************" << std::endl;
  std::cout << std::endl;
}

void
printDeviceList ()
{
	typedef boost::shared_ptr<pcl::RealSenseGrabber> RealSenseGrabberPtr;
	std::vector<RealSenseGrabberPtr> grabbers;
	std::cout << "Connected devices: ";
  	boost::format fmt ("\n  #%i  %s");
  	while (true)
  	{
    	try
    	{
     		grabbers.push_back (RealSenseGrabberPtr (new pcl::RealSenseGrabber));
      		std::cout << boost::str (fmt % grabbers.size () % grabbers.back ()->getDeviceSerialNumber ());
    	}
    	catch (pcl::io::IOException& e)
    	{
      		break;
    	}
  	}
  	if (grabbers.size ())
    	std::cout << std::endl;
  	else
    	std::cout << "none" << std::endl;
}


template <typename PointT>
class RealSenseViewer
{
	public:
		typedef pcl::PointCloud<PointT> PointCloudT;

		RealSenseViewer (pcl::RealSenseGrabber& grabber)
    	: grabber_ (grabber)
    	, viewer_ ("RealSense Viewer")
    	, grabber_second_ (grabber)
    	, viewer_second_ (viewer_)
    	{
      		//viewer_.registerKeyboardCallback (&RealSenseViewer::keyboardCallback, *this);
    	}

		RealSenseViewer (pcl::RealSenseGrabber& grabber, pcl::RealSenseGrabber& grabber_second)
    	: grabber_ (grabber)
    	, viewer_ ("RealSense Viewer One")
    	,grabber_second_ (grabber_second)
    	, viewer_second_ ("RealSense Viewer Two")
    	{
      		//viewer_.registerKeyboardCallback (&RealSenseViewer::keyboardCallback, *this);
    	}

    	~RealSenseViewer ()
    	{
      		connection_.disconnect ();
    	}

    	void
    	run ()
    	{
      		boost::function<void (const typename PointCloudT::ConstPtr&)> f = boost::bind (&RealSenseViewer::cloudCallback, this, _1);
      		connection_ = grabber_.registerCallback (f);
      		grabber_.start ();
      		int grabber_switch = 0;
      		while (!viewer_.wasStopped ())
      			{
        		if (new_cloud_)
        			{
          			boost::mutex::scoped_lock lock (new_cloud_mutex_);
          			if (!viewer_.updatePointCloud (new_cloud_, "cloud"))
          				{
            				viewer_.addPointCloud (new_cloud_, "cloud");
            				viewer_.resetCamera ();
          				}
          			//grabber_switch++;	
          			displaySettings ();
          			last_cloud_ = new_cloud_;
          			new_cloud_.reset ();
        			}
        		viewer_.spinOnce (1, true);
        		/*if(grabber_switch == 100)
        			grabber_.stop ();
        		if(grabber_switch >= 100)
        			grabber_switch++;
        		if(grabber_switch == 200)
        			grabber_.start ();*/
        		//printf("grabber_switch: %d\t", grabber_switch);
      			}
      		grabber_.stop ();
    	}

    	void
    	start ()
    	{
      		boost::function<void (const typename PointCloudT::ConstPtr&)> f = boost::bind (&RealSenseViewer::cloudCallback, this, _1);
      		connection_ = grabber_.registerCallback (f);
      		grabber_.start ();

      		boost::function<void (const typename PointCloudT::ConstPtr&)> f_second_ = boost::bind (&RealSenseViewer::cloudCallback_second, this, _1);
      		connection_second_ = grabber_second_.registerCallback (f_second_);
      		grabber_second_.start ();

      		int grabber_switch = 0;
      		while(!viewer_.wasStopped () || !viewer_second_.wasStopped ())
      		{

      		if (!viewer_.wasStopped ())
      			{
        		if (new_cloud_)
        			{
          			boost::mutex::scoped_lock lock (new_cloud_mutex_);
          			if (!viewer_.updatePointCloud (new_cloud_, "cloud"))
          				{
            				viewer_.addPointCloud (new_cloud_, "cloud");
            				viewer_.resetCamera ();
          				}
          			displaySettings ();
          			last_cloud_ = new_cloud_;
          			new_cloud_.reset ();
        			}
        			
        		viewer_.spinOnce (1, true);
        		}
        	if (!viewer_second_.wasStopped ())
      			{
        		if (new_cloud_second_)
        			{
          			boost::mutex::scoped_lock lock (new_cloud_mutex_second_);
          			if (!viewer_second_.updatePointCloud (new_cloud_second_, "cloud_second_"))
          				{
            				viewer_second_.addPointCloud (new_cloud_second_, "cloud_second_");
            				viewer_second_.resetCamera ();
          				}
          			displaySettings_second ();
          			last_cloud_second_= new_cloud_second_;
          			new_cloud_second_.reset ();
        			}
        			
        		viewer_second_.spinOnce (1, true);
      			}
      		}
      		grabber_.stop ();
      		grabber_second_.stop ();
    	}

    private:

    	void
    	cloudCallback (typename PointCloudT::ConstPtr cloud)
    	{
      	if (!viewer_.wasStopped ())
      		{
        	boost::mutex::scoped_lock lock (new_cloud_mutex_);
        	new_cloud_ = cloud;
      		}
    	}

    	void
    	cloudCallback_second (typename PointCloudT::ConstPtr cloud)
    	{
      	if (!viewer_second_.wasStopped ())
      		{
        	boost::mutex::scoped_lock lock (new_cloud_mutex_second_);
        	new_cloud_second_ = cloud;
      		}
    	}

    void
    keyboardCallback (const pcl::visualization::KeyboardEvent& event, void*)
    {
      if (event.keyDown ())
      {
        if (event.getKeyCode () == 's')
        {
          boost::format fmt ("DS_%s_%u.pcd");
          std::string fn = boost::str (fmt % grabber_.getDeviceSerialNumber ().c_str () % last_cloud_->header.stamp);
          pcl::io::savePCDFileBinaryCompressed (fn, *last_cloud_);
          pcl::console::print_info ("Saved point cloud: ");
          pcl::console::print_value (fn.c_str ());
          pcl::console::print_info ("\n");
        }
        displaySettings ();
      }
    }

    void displaySettings ()
    {
      const int dx = 5;
      const int dy = 14;
      const int fs = 10;
      boost::format name_fmt ("text%i");
      std::vector<boost::format> entries;
      // Framerate
      entries.push_back(boost::format ("framerate: %.1f") % grabber_.getFramesPerSecond ());

      for (size_t i = 0; i < entries.size (); ++i)
      {
        std::string name = boost::str (name_fmt % i);
        std::string entry = boost::str (entries[i]);
        if (!viewer_.updateText (entry, dx, dy + i * (fs + 2), fs, 1.0, 1.0, 1.0, name))
          viewer_.addText (entry, dx, dy + i * (fs + 2), fs, 1.0, 1.0, 1.0, name);
      }
    }

    void displaySettings_second ()
    {
      const int dx = 5;
      const int dy = 14;
      const int fs = 10;
      boost::format name_fmt ("text%i");
      std::vector<boost::format> entries;
      // Framerate
      entries.push_back(boost::format ("framerate: %.1f") % grabber_second_.getFramesPerSecond ());

      for (size_t i = 0; i < entries.size (); ++i)
      {
        std::string name = boost::str (name_fmt % i);
        std::string entry = boost::str (entries[i]);
        if (!viewer_second_.updateText (entry, dx, dy + i * (fs + 2), fs, 1.0, 1.0, 1.0, name))
          viewer_second_.addText (entry, dx, dy + i * (fs + 2), fs, 1.0, 1.0, 1.0, name);
      }
    }

    pcl::RealSenseGrabber& grabber_;
    pcl::visualization::PCLVisualizer viewer_;
    boost::signals2::connection connection_;

    mutable boost::mutex new_cloud_mutex_;
    typename PointCloudT::ConstPtr new_cloud_;
    typename PointCloudT::ConstPtr last_cloud_;


    pcl::RealSenseGrabber& grabber_second_;
    pcl::visualization::PCLVisualizer viewer_second_;
    boost::signals2::connection connection_second_;
    mutable boost::mutex new_cloud_mutex_second_;
    typename PointCloudT::ConstPtr new_cloud_second_;
    typename PointCloudT::ConstPtr last_cloud_second_;



};

int
main (int argc, char** argv)
{
  print_info ("Viewer for RealSense devices (run with --help for more information)\n", argv[0]);

  if (find_switch (argc, argv, "--help") || find_switch (argc, argv, "-h"))
  {
    printHelp (argc, argv);
    return (0);
  }

  if (find_switch (argc, argv, "--list") || find_switch (argc, argv, "-l"))
  {
    printDeviceList ();
    return (0);
  }

  bool xyz_only = find_switch (argc, argv, "--xyz");

  std::string device_id;

  if (argc == 1 ||             // no arguments
     (argc == 2 && xyz_only))  // single argument, and it is --xyz
  {
    device_id = "";
    print_info ("Creating a grabber for the first available device\n");
  }
  else
  {
    device_id = argv[argc - 1];
    print_info ("Creating a grabber for device \"%s\"\n", device_id.c_str ());
  }

  try
  {
    pcl::RealSenseGrabber grabber (device_id);
    if (xyz_only)
    {
      RealSenseViewer<pcl::PointXYZ> viewer (grabber);
      viewer.run ();
    }
    else
    {
      //Signle viewer
      RealSenseViewer<pcl::PointXYZRGBA> viewer (grabber);
      viewer.run ();

      //Two viewers
      /*pcl::RealSenseGrabber grabber1 (device_id);
      RealSenseViewer<pcl::PointXYZRGBA> viewer (grabber, grabber1);
      viewer.start ();*/
    }
  }
  catch (pcl::io::IOException& e)
  {
    print_error ("Failed to create a grabber: %s\n", e.what ());
    return (1);
  }

  return (0);
}
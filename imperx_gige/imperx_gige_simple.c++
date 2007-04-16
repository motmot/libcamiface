#include <CyXMLDocument.h>
#include <CyConfig.h>
#include <CyGrabber.h>
#include <CyCameraRegistry.h>
#include <CyCameraInterface.h>
#include <CyImageBuffer.h>
#include <CyDeviceFinder.h>
#include <stdio.h>

#define _dping(m)					\
  printf("%s (%d): %s\n",__FILE__,__LINE__,(m))

int main( void )
{
  _dping("");

    // We now work with an adapter identifier, which is basically a MAC address
    // For this example, we something need a valid ID so we will get one
    // from the CyAdapterID class
    CyAdapterID lAdapterID;
  _dping("");
    CyAdapterID::GetIdentifier( CyConfig::MODE_DRV, 0, lAdapterID );
  _dping("");

    // Step 0 (optional): Set the IP address on the module.  Since the
    // XML file may have a forced address, we need to send it to the module.
    // You may skip this step if:
    // - you have a BOOTP server that sets the module's address
    // - if you have a direct connection from an performance driver card
    //   to the iPORT and that your XML file uses an empty address.
//    CyDeviceFinder lFinder;
//    if ( lFinder.SetIP( CyConfig::MODE_DRV,  // or CyConfig::MODE_UDP
//                        lAdapterID,
//                        "00-50-C2-1D-70-05", // the MAC address of the module to update
//                        "[192.168.2.35]" )   // The address to set
//         != CY_RESULT_OK )
//    {
//        // error
//        return 1;
//    }

    // Step 1:  Open the configuration file.  We create a XML 
    // document with the name of the file as parameter.
    CyXMLDocument lDocument( "IPX-2M30-L.xml" );
    _dping("Testing for IPX-2M30-L.xml");
    if ( lDocument.LoadDocument() != CY_RESULT_OK )
    {
        _dping("IPX-2M30-L.xml not found");
        // error
        return 1;
    }
    _dping("Found IPX-2M30-L.xml");

    // Step 2a: Create a copnfiguration object and fill it from the XML document.
    CyConfig lConfig;
    if ( lConfig.LoadFromXML( lDocument ) != CY_RESULT_OK )
    {
        // error
        return 1;
    }

    // Step 2b:  We could also fill the config by calling AddDevice and
    // the Set... functions to force values.
//     lConfig.AddDevice();
//     lConfig.SetAccessMode( CyConfig::MODE_DRV );
//     lConfig.SetAddress( "" ); // direct connection
//     lConfig.SetAdapterID( lAdapterID );  // first Pleora Ethernet card
//     lConfig.SetDeviceType( "Standard CameraLink Camera" );
//     lConfig.SetName( "DeviceName" );



    // Step 3a: Set the configuration on the entry to connect to.
    // In this case, we only have one entry, so index 0, is good.
    // Select the device you want to use
    lConfig.SetIndex( 0 );

    // Step 3b: Set the configuration on the entry to connect to.
    // Here, we know the name of the device, so we search for it.
    // if ( lConfig.FindDeviceByName( "DeviceName" ) != CY_RESULT_OK )
    // {
    //     // error
    //     return 1;
    // }



    // Step 4: Connect to the grabber.  It will use the currently
    // selected entry in the config object, hence step 3.

    CyGrabber lGrabber;
    if ( lGrabber.Connect( lConfig ) != CY_RESULT_OK )
    {
        // error
        return 1;
    }



    // Step 5: Create a camera object on top of the grabber.  This camera
    // object takes care of configuring both the iPORT and the camera.

    // Find the camera type from the configuration
    char lCameraType[128];
    lConfig.GetDeviceType( lCameraType, sizeof( lCameraType ) );

    // Find the camera in the registry
    CyCameraRegistry lReg;
    if ( lReg.FindCamera( lCameraType ) != CY_RESULT_OK )
    {
        // error
        return 1;
    }

    // Create the camera.  The previous operation placed the registry 
    // internal settings on the found camera.  The next step will create
    // a camera object of that type.
    CyCameraInterface* lCamera = 0;
    if ( lReg.CreateCamera( &lCamera, &lGrabber ) != CY_RESULT_OK )
    {
        // error
        return 1;
    }



    // Step 6: Load the camera settings from the XML document

    if ( lCamera->LoadFromXML( lDocument ) != CY_RESULT_OK )
    {
        // error
        delete lCamera;
        return 1;
    };



    // Step 7: Send the settings to iPORT and the camera

    if ( lCamera->UpdateToCamera() != CY_RESULT_OK )
    {
        // error
        delete lCamera;
        return 1;
    };



    // Step 8:  Create a buffer for grabbing images.

    // Get some information from the camera
    unsigned short lSizeX, lSizeY, lDecimationX, lDecimationY, lBinningX, lBinningY;
    CyPixelTypeID lPixelType;
    lCamera->GetSizeX( lSizeX );
    lCamera->GetSizeY( lSizeY );
    lCamera->GetDecimationX( lDecimationX );
    lCamera->GetDecimationY( lDecimationY );
    lCamera->GetBinningX( lBinningX );
    lCamera->GetBinningY( lBinningY );
    lCamera->GetPixelType( lPixelType );

	if ( ( lDecimationX != 0 ) && ( lDecimationY != 0 ) && ( lBinningX != 0 ) && ( lBinningY != 0 ) )
	{
        lSizeX = (( lSizeX / lBinningX ) + (( lSizeX % lBinningX ) ? 1 : 0));
        lSizeX = (( lSizeX / lDecimationX ) + (( lSizeX % lDecimationX ) ? 1 : 0));
        lSizeY = (( lSizeY / lBinningY ) + (( lSizeY % lBinningY ) ? 1 : 0));
        lSizeY = (( lSizeY / lDecimationY ) + (( lSizeY % lDecimationY ) ? 1 : 0));
    }

    // Create the buffer.
    CyImageBuffer lBuffer( lSizeX, lSizeY, lPixelType );



    // Step 9: Grab an image
   
    // In this case, it does not change anything, but some future camera
    // module may need to perform initialization before the grabbing, so
    // grabbing through the camera is preferred.

    if ( lCamera->Grab(  CyChannel( 0 ), // always this for now,
                         lBuffer,
                         0 ) != CY_RESULT_OK )
    {
        // error
        delete lCamera; // to avoid memory leak
        return 1;
    }


    // Step 10: Getting the buffer pointer from the CyBuffer class

    const unsigned char* lPtr;
    unsigned long lSize;
    if ( lBuffer.LockForRead( (void**) &lPtr, &lSize, CyBuffer::FLAG_NO_WAIT ) == CY_RESULT_OK )
    {
        // Now, lPtr points to the data and lSize contains the number of bytes available.

        // Also, the GetRead...() methods are available to inquire information
        // about the buffer.

        // Now release the buffer
        lBuffer.SignalReadEnd();
    }
    

    // Step 11: Display the grabber image
    // NOTE: The display needs to be used in a MFC application, so it
    // will be commented out.

    printf("Hello world.\n");
	//CyDisplay lDisplay;
    //lDisplay.Open( &lBuffer, 0 );
    //lDisplay.Display( &lBuffer, 0 );
    //lDisplay.Close();



    // Step 12: Ending.  The camera object is the only pointer we use,
    // so destroy it to avoid a memory leak.

    delete lCamera;
    return 0;
}

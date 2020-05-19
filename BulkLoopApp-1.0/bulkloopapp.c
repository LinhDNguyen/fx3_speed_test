/* 
* bulkloopapp.c -- Program for testing USB communication using libusb 1.0
* Cypress semiconductor. 2011
*/

#include <stdio.h>
#include <string.h>
#include "bulkloopapp.h"

//handle of selected device to work on
libusb_device *device;
libusb_device_handle *dev_handle;

unsigned char glInEpNum=0;
unsigned char glOutEpNum=0;
unsigned int  glInMaxPacketSize=0;
unsigned int  glOutMaxPacketSize=0;
 
void  device_info()
{
    struct libusb_config_descriptor *config;
    const struct libusb_interface *inter;
    const struct libusb_interface_descriptor *interdesc;
    const struct libusb_endpoint_descriptor *epdesc;
    int i,j,k;

    i = j = k = 0;
    libusb_get_config_descriptor(device, 0, &config);
    printf("\n\n");
    printf("----------------------------------------------------------------------------------------");
    printf("\nNumber of interfaces %15d",(int) config->bNumInterfaces);

    for(i=0;i<(int)config->bNumInterfaces;i++)
    {
        inter = &config->interface[i];
        printf("\nNumber of alternate settings %7d",inter->num_altsetting);
        printf("\n");
        for(j=0;j<inter->num_altsetting;j++)
        {
            interdesc = &inter->altsetting[j];
            printf("\ninterface Number %d, Alternate Setting %d,Number of Endpoints %d\n",\
                    (int)interdesc->bInterfaceNumber,(int) interdesc->bAlternateSetting,(int) interdesc->bNumEndpoints);
            for(k=0;k<(int) interdesc->bNumEndpoints;k++)
            {
                epdesc = &interdesc->endpoint[k];
                printf("\nEP Address %5x\t",(int) epdesc->bEndpointAddress);
                switch((epdesc->bmAttributes)&0x03)
                {
                    case 0:
                        printf("BULK IN Endpoint");
                        break;
                    case 1:
                        if((epdesc->bEndpointAddress) & 0x80)
                            printf("Isochronous IN Endpoint");
                        else
                            printf("Isochronous OUT Endpoint");
                        break;
                    case 2:
                        if((epdesc->bEndpointAddress) & 0x80)
                            printf("Bulk IN Endpoint");
                        else
                            printf("Bulk OUT Endpoint");
                        break;
                    case 3:
                        if((epdesc->bEndpointAddress) & 0x80)
                            printf("Interrupt IN Endpoint");
                        else
                            printf("Interuppt OUT Endpoint");
                        break;
                }
                printf("\n");
            }
        }
    }
    printf("----------------------------------------------------------------------------------------");
    printf("\n\n");

    libusb_free_config_descriptor(config);
}

int get_ep_info(void)
{
	
	int err;	
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;
    const struct libusb_interface *inter;
    const struct libusb_interface_descriptor *interdesc;
    const struct libusb_endpoint_descriptor *epdesc;
    int k;

	//detect the bulkloop is running from VID/PID 
    err = libusb_get_device_descriptor(device, &desc);
    if (err < 0) 
    {
        printf("\n\tFailed to get device descriptor for the device, returning");
        return 1;
    }

    if((desc.idVendor == 0x04b4) && (desc.idProduct == 0x00F0))
        printf("\n\nFound FX3 bulkloop device, continuing...\n");
	else if((desc.idVendor == 0x04b4) && (desc.idProduct == 0x1004))
        printf("\n\nFound FX2LP bulkloop device, continuing...\n");

    else
    {
        printf("\n ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");  
        printf("\nFor seeing whole bulkloop action from HOST -> TARGET ->HOST, Please run correct firmware on TARGET and restart this program");
        printf("\n -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");  
        return 1;
    }

	k = 0;
    libusb_get_config_descriptor(device, 0, &config);
	/* this device has only one interface */
	if(((int) config->bNumInterfaces)>1)
	{
		printf("to many interfaces\n");		
		return 1;
	}    

	
	/* Interface has only two bulk Endpoints */
    inter = &config->interface[0];
	interdesc = &inter->altsetting[0];
	if(((int) interdesc->bNumEndpoints)>2)
	{
		printf("to many Endpoints\n");		
		return 1;
	}    

	/*find Endpoint address, direction and size*/
	for(k=0;k<(int) interdesc->bNumEndpoints;k++)
	{

		epdesc = &interdesc->endpoint[k];
	
		if((epdesc->bEndpointAddress) & 0x80)
		{
			printf("Bulk IN Endpoint 0x%02x\n", epdesc->bEndpointAddress);
			glInEpNum=epdesc->bEndpointAddress;
			glInMaxPacketSize=epdesc->wMaxPacketSize;		
		}
		else
		{
			printf("Bulk OUT Endpoint 0x%02x\n", epdesc->bEndpointAddress);
			glOutEpNum=epdesc->bEndpointAddress;
			glOutMaxPacketSize=epdesc->wMaxPacketSize;		
		}
	} 
    
	if(glOutMaxPacketSize!=glInMaxPacketSize)
	{
		printf("\nEndpoints size is not maching\n");		
		return 1;

	}
    libusb_free_config_descriptor(config);
	return 0;


}

void  bulk_transfer()
{
    int err;
    int i;
    int usr_choice,data_byte;
    int transffered_bytes; //actual transffered bytes
    unsigned char out_data_buf[1024];
    unsigned char in_data_buf[1024];
    struct libusb_device_descriptor desc;

    printf("\n-------------------------------------------------------------------------------------------------");  
    printf("\nThis function is for testing the bulk transfers. It will write on OUT endpoint and read from IN endpoint");
    printf("\n-------------------------------------------------------------------------------------------------");      

    //detect the bulkloop is running from VID/PID 
    err = libusb_get_device_descriptor(device, &desc);
    if (err < 0) 
    {
        printf("\n\tFailed to get device descriptor for the device, returning");
        return;
    }


	if(get_ep_info())
	{
		printf("\n can not get EP Info; returning");
        return;
	}

  
    // While claiming the interface, interface 0 is claimed since from our bulkloop firmware we know that. 
    err = libusb_claim_interface (dev_handle, 0);
    if(err)
    {
        printf("\nThe device interface is not getting accessed, HW/connection fault, returning");
        return;
    }

    //Create a buffer of users choice
    printf("\n-------------------------------------------------------------------------------------------------");  
    printf("\n\nPlease select %d byte data to be transffered :-", glOutMaxPacketSize);
    printf("\n1. Incrementing pattern");
    printf("\n2. Decrementing pattern");
    printf("\n3. Same data in all bytes");
    /*printf("\n4. User defined data for every byte");*/
    do {
        printf("\n\nEnter the choice [e.g 2]        :");
        err = scanf("%d",&usr_choice);
        if (err == 0)
        {
            err = scanf ("%s", in_data_buf);
            err = 0;
        }
    } while (err != 1);
    do {
        printf("\nNow please enter the data byte  :");
        err = scanf("%d",&data_byte);
        if (err == 0)
        {
            err = scanf ("%s", in_data_buf);
            err = 0;
        }
    } while (err != 1);
    printf("\n---------------------------------------------------------------------------------------------------\n");        
    
    switch(usr_choice)
    {
         case 1:
            printf("\nFilling the buffer...");
            for(i=0; i < (int)glOutMaxPacketSize; i++)
            {    
                out_data_buf[i] = (unsigned char)data_byte;
                data_byte++;
            }        
            break;
        case 2:
            printf("\nFilling the buffer...");
            for(i=0; i < (int)glOutMaxPacketSize; i++)
            {    
                out_data_buf[i] = (unsigned char)data_byte;
                //if(data_byte != 0)
                    data_byte--;
            }        
            break;
        /*case 4:
            printf("\nNot implemented yet, will fill buffer withy same data");*/
        case 3:
            printf("\nFilling the buffer...");
            for(i=0; i < (int)glOutMaxPacketSize; i++)
                out_data_buf[i] = (unsigned char)data_byte;
            break;
        default:
            printf("\nSince not entered correct choice between 1 to 4, ");
            printf("\nfilling the buffer with entered data byte ");
            printf("\nFilling the buffer...");
            for(i=0; i < (int)glOutMaxPacketSize; i++)
                out_data_buf[i] = (unsigned char)data_byte;
     }
     printf("\nTransffering %d bytes from HOST(PC) -> TARGET(Bulkloop device)", glOutMaxPacketSize);
     err = libusb_bulk_transfer(dev_handle,glOutEpNum,out_data_buf,glOutMaxPacketSize,&transffered_bytes,100); 
     if(err)
     {
        printf("\nBytes transffres failed, err: %d transffred bytes : %d",err,transffered_bytes); 
        return;
     }   
     transffered_bytes = 0;
     printf("\nReturning %d bytes from TARGET(Bulkloop device) -> HOST(PC)", glInMaxPacketSize);
     err = libusb_bulk_transfer(dev_handle,glInEpNum,in_data_buf,glInMaxPacketSize,&transffered_bytes,100);
     if(err)
     {
        printf("\nreturn transffer failed, err: %d transffred bytes : %d",err,transffered_bytes);
        return;
     }     
     printf("\n\n------------------------------------------------------------------------------------------------------------------");
     printf("\n\nBulkloop transfers completed successfully:");
     printf("\nData Transffered: %d bytes \n\n", transffered_bytes);
     for(i=0; i < (int)transffered_bytes; i++)
        printf("%d\t",out_data_buf[i]);
     printf("\n\nData Received: %d bytes\n\n", transffered_bytes);
     for(i=0; i < (int)transffered_bytes; i++)
        printf("%d\t",in_data_buf[i]);
     printf("\n\nPlease check if Bytes Transferred are same as Bytes Received through Bulkloop process");
     printf("\n\n------------------------------------------------------------------------------------------------------------------\n\n");     
     
     
     //release the interface claimed erlier
     err = libusb_release_interface (dev_handle, 0);     
     if(err)
     {
        printf("\nThe device interface is not getting released, if system hangs please disconnect the device, returning");
        return;
     }
}

//Print info about the buses and devices.
int print_info(libusb_device **devs)
{	
	int i,j;
	int busNo, devNo;
    int err = 0;

    i = j = 0;
    
    printf("\nList of Buses and Devices attached :- \n\n");    
	while ((device = devs[i++]) != NULL) 
	{
		struct libusb_device_descriptor desc;

		int r = libusb_get_device_descriptor(device, &desc);
		if (r < 0) 
		{
			printf("\n\tFailed to get device descriptor for the device, returning");
			return err;
	    }

		printf("Bus: %03d Device: %03d: Device Id: %04x:%04x\n",
			libusb_get_bus_number(device),libusb_get_device_address(device),
			desc.idVendor, desc.idProduct );
	}
	printf("\nChoose the device to work on, From bus no. and device no. :-");
	printf("\nEnter the bus no : [e.g. 2]      :");
	err = scanf("%d",&busNo);
	printf("Enter the device no : [e.g. 5]  :");
	err = scanf("%d",&devNo);	
	
	while ((device = devs[j++]) != NULL) 
	{
		struct libusb_device_descriptor desc;

		libusb_get_device_descriptor(device, &desc);
	    if ( busNo == libusb_get_bus_number(device))
	    {
	        if ( devNo == libusb_get_device_address(device))
	        {
                printf("\n --------------------------------------------------------------------------------------------");  
	            printf("\nYou have selected USB device : %04x:%04x:%04x\n\n", \
	            desc.idVendor, desc.idProduct,desc.bcdUSB);
	            return 0;
	         }
	    }
	}    

    printf("\nIllegal/nonexistant device, please restart and enter correct busNo, devNo");
    return err;
}

// Show command line help
void show_help()
{
	// Show the help to user with all the valid argument format	
	//printf("\n\t USBTestApp -- HELP \n\n");
	return;
}

// scan the arguments givan by user
void scan_arg()
{
	//If arguments not correct show help
	show_help();	
	return;
}

int main()
{
	int err;
	int usr_choice;
	ssize_t cnt;  // USB device count
	libusb_device **devs; //USB devices
	//scan the arguments amd make sence out of them
	scan_arg();

	//initialise the libusb library
    err = libusb_init(NULL);
    if(err)
    {
        printf("\n\t The libusb library failed to initialise, returning");
        return(err);
     }   

    //Detect all devices on the USB bus.
    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0)
    {
        printf("\n\t No device is connected to USB bus, returning ");
        return (int)cnt;
    }
    
    //print all devices on the bus 
    err = print_info(devs);
    if(err)
    {
        printf("\nEnding the program, due to error\n");
        return(err);
     }   
    
    //open the device handle for all future operations
    err = libusb_open (device, &dev_handle);
    if(err)
    {
        printf("\nThe device is not opening, check the error code, returning\n");
        return err;
    }
    
    //since the use of device list is over, free it
    libusb_free_device_list(devs, 1);
    
    do{
        //what user want to do with selected device
        printf("What do you want to do ?");
        printf("\n1. Give information about the device.");
        printf("\n2. Do the bulk transfer");
        printf("\n3. Exit");
        printf("\n\nEnter the choice [e.g 3]   :");
        err = scanf("%d",&usr_choice);
       
        switch(usr_choice)
        {
            case 1:
                device_info();
                break;
            case 2:
                bulk_transfer();
                break;
            case 3:
                printf("\nExiting");
                return 0;                
            default:
                printf("\nPlease enter the correct choice between 1 to 3\n");
        }
    }while(1); // loop continuously till exit
             
    //All tasks done close the device handle
    libusb_close(dev_handle);
    
    //Exit from libusb library
    libusb_exit(NULL);
    
	return 0;
}

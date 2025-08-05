/*

Some description...

*/

// Standard
#include <iostream>
#include <memory>
#include <fstream>
#include <time.h>
#include <filesystem>

// ROOT
#include <TFile.h>
#include <TTree.h>
#include <TBufferJSON.h>
#include <TSystem.h>

// Unpackers
#include <unpackers/common/Logger.hh>
#include <unpackers/wfd5/WFD5EventUnpacker.hh>

// WFD5 Data Products
#include <data_products/wfd5/WFD5Header.hh>
#include <data_products/wfd5/WFD5ChannelHeader.hh>
#include <data_products/wfd5/WFD5WaveformHeader.hh>
#include <data_products/wfd5/WFD5Waveform.hh>
#include <data_products/wfd5/WFD5ODB.hh>

// Reco
#include <reco/wfd5/WFD5OutputManager.hh>
#include <reco/common/EventStore.hh>
#include <reco/common/ConfigHolder.hh>
// RecoStages
#include <reco/common/RecoManager.hh>
#include <reco/wfd5/JitterCorrector.hh>
// Services
#include <reco/common/ServiceManager.hh>
#include <reco/wfd5/TemplateService.hh>

#include <string>
#include <sstream>
// #include <nlohmann/json.hpp>

using unpackers::LoggerHolder;

int main(int argc, char *argv[])
{
    
    // -----------------------------------------------------------------------------------------------
    // Parse command line arguments

    // Parse the arguments
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " configFile inputFileName verbosity" << std::endl;
        return 1;
    }
    // config file name
    std::string config_file_name = argv[1];

    // input file name
    std::string input_file_name = argv[2];

    int verbosity = std::atoi(argv[3]);

    // Check if input file exists
    if (!std::filesystem::exists(input_file_name)) {
        std::cerr << "Input file doesn't exist. Could not find " << input_file_name << std::endl;
        return 1;
    }

    // Check if config file exists
    if (!std::filesystem::exists(config_file_name)) {
        std::cerr << "Config file doesn't exist. Could not find " << config_file_name << std::endl;
        return 1;
    }

    // Parse the run/subrun number from the file name
    std::cout << "-> main: Reading in file: " << input_file_name << std::endl;
    int run = 0;
    int subrun = 0;
    try
    {
        size_t start1 = input_file_name.find("run") + 3;
        size_t end1 = input_file_name.find("_", start1);
        std::string num1 = input_file_name.substr(start1, end1 - start1);

        // Find the start and end of the second number
        size_t start2 = end1 + 1;
        size_t end2 = input_file_name.find(".", start2);
        std::string num2 = input_file_name.substr(start2, end2 - start2);

        // Convert strings to integers
        std::stringstream(num1) >> run;
        std::stringstream(num2) >> subrun;
        std::cout << "-> main: Run/Subrun of this file: " << run << " / " << subrun << std::endl;
    }
    catch (...)
    {
        std::cerr << "-> main: Warning: Unable to parse run/subrun from file" << std::endl;
    }

    // output file name
    std::string output_file_name;
    output_file_name = input_file_name.substr(input_file_name.find_last_of("/\\") + 1);
    output_file_name = output_file_name.substr(0, output_file_name.find_first_of('.')) + ".root";
    std::cout << "-> main: Output file: " << output_file_name << std::endl;

    // Set verbosity for unpacker
    LoggerHolder::getInstance().SetVerbosity(verbosity);

    // End of parsing command line arguments
    // -----------------------------------------------------------------------------------------------


    // -----------------------------------------------------------------------------------------------
    // Set up the various managers etc.

    // Create configuration holder
    std::shared_ptr<reco::ConfigHolder> configHolder = std::make_shared<reco::ConfigHolder>();
    configHolder->LoadFromFile(config_file_name);
    configHolder->SetRunSubrun(run, subrun);

    // Create the output manager
    reco::OutputManager* outputManager = new reco::WFD5OutputManager(output_file_name);
    outputManager->Configure(configHolder);

    // Create the midas event unpacker
    unpackers::EventUnpacker* eventUnpacker = new unpackers::WFD5EventUnpacker();

    // Create the EventStore to hold all the data products during processing
    reco::EventStore eventStore;

    // Create the service manager
    reco::ServiceManager serviceManager;
    serviceManager.Configure(configHolder);

    // Create the reco manager
    reco::RecoManager recoManager;
    recoManager.Configure(configHolder, serviceManager);

    // Create some histograms
    auto hist = std::make_shared<TH1D>("energy", "Energy Spectrum", 100, 0, 1000);
    eventStore.putHistogram("energy", hist);

    // Create the Midas reader
    TMReaderInterface *mReader = TMNewReader(input_file_name.c_str());

    // -----------------------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------------------
    // Time to start the midas event loop
    int nTotalMidasEvents = 0;
    int nSkippedMidasEvents= 0;

    // loop over the events
    while (true) {
        TMEvent *thisEvent = TMReadEvent(mReader);
        //if (!thisEvent || nTotalMidasEvents > 100 )
        if (!thisEvent)
        {
            // Reached end of the file. Clean up and break
            delete thisEvent;
            break;
        }

        if (thisEvent->serial_number % 100 == 0) {
            std::cout << "-> main: event_id: " << thisEvent->event_id << ", serial number: " << thisEvent->serial_number << std::endl;
        }
        
        int event_id = thisEvent->event_id;

        // Check if this is an internal midas event
        if (unpackers::IsHeaderEvent(thisEvent)) {
            // Check if this is a BOR (begin of run)
            if (event_id == 32768) {
                // This is a begin of run event
                // and contains an odb dump
                std::vector<char> data = thisEvent->data;
                std::string odb_dump(data.begin(), data.end());
                std::size_t pos = odb_dump.find('{');
                if (pos != std::string::npos) {
                    odb_dump.erase(0, pos);  // Keep the '{'
                }
                // std::cout << odb_dump << std::endl;
                // nlohmann::json j = nlohmann::json::parse(odb_dump);
                // std::cout << j.dump(4) << std::endl;
                // make the ODB data product
                std::shared_ptr<dataProducts::DataProduct> wfd5_odb = std::make_shared<dataProducts::WFD5ODB>(odb_dump);
                eventStore.put_odb(wfd5_odb);
                outputManager->WriteODB(eventStore);
            }
            delete thisEvent;
            continue;
        }

        thisEvent->FindAllBanks();
        // thisEvent->PrintBanks();
        // auto bank = thisEvent->FindBank("AD%0");
        // std::cout << thisEvent->BankToString(bank) << std::endl;

        // only unpack events with id 1
        if (event_id == 1) {
            nTotalMidasEvents++;
            // std::cout << "Processing event " << nTotalMidasEvents << std::endl;
            
            // Unpack the event; this is done in a loop in case there are multiple "trigger" events in the midas event
            unpackers::unpackingStatus status = unpackers::unpackingStatus::Failure;
            while ( (status = eventUnpacker->UnpackEvent(thisEvent)) == unpackers::unpackingStatus::SuccessMore) {

                // Put the unpacked data into the event store
                eventStore.clear();  // clear previous event's data
                eventStore.put("unpacker","WFD5HeaderCollection", eventUnpacker->GetNextPtrCollection<dataProducts::WFD5Header>("WFD5HeaderCollection"));
                eventStore.put("unpacker","WFD5ChannelHeaderCollection", eventUnpacker->GetNextPtrCollection<dataProducts::WFD5ChannelHeader>("WFD5ChannelHeaderCollection"));
                eventStore.put("unpacker","WFD5WaveformHeaderCollection", eventUnpacker->GetNextPtrCollection<dataProducts::WFD5WaveformHeader>("WFD5WaveformHeaderCollection"));
                eventStore.put("unpacker","WFD5WaveformCollection", eventUnpacker->GetNextPtrCollection<dataProducts::WFD5Waveform>("WFD5WaveformCollection"));

                // Run reconstruction stages
                try {
                    recoManager.Run(eventStore, serviceManager);
                } catch (const std::exception& e) {
                    std::cerr << "Error during reconstruction: " << e.what() << std::endl;
                    return 1;
                }

                // Fill the output tree with the event data
                outputManager->FillEvent(eventStore);
            }

            // Clean up the event now that we are done with it
            delete thisEvent;
            continue;
            
        } // end if event id = 1

    } // end loop over events

    outputManager->WriteHistograms(eventStore);

    // clean up
    delete eventUnpacker;
    delete outputManager;
    delete mReader;

    std::cout << "-> main: Skipped " << nSkippedMidasEvents << "/" << nTotalMidasEvents << " midas events" << std::endl;

    std::cout << "-> main: All done!" << std::endl;
    return 0;
}

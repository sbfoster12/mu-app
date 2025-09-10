/*

This provides the main method for the nearline executable for processing WFD5 data.

*/

// Standard
#include <iostream>
#include <memory>
#include <fstream>
#include <time.h>
#include <filesystem>
#include <chrono>

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
// Services
#include <reco/common/ServiceManager.hh>

#include <string>
#include <sstream>
// #include <nlohmann/json.hpp>

using unpackers::LoggerHolder;

int main(int argc, char *argv[])
{
    // Start the clock!
    auto start = std::chrono::high_resolution_clock::now();
    
    // -----------------------------------------------------------------------------------------------
    // Parse command line arguments

    // Parse the arguments
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " configFile inputFileName outputFileName[optional]" << std::endl;
        return 1;
    }
    // config file name
    std::string config_file_name = argv[1];

    // input file name
    std::string input_file_name = argv[2];

    // Check if input file exists
    if (!std::filesystem::exists(input_file_name)) {
        std::cerr << "Input file doesn't exist. Could not find " << input_file_name << std::endl;
        return 1;
    }

    // Check if config file exists
    std::string config_file_path = "";
    if (config_file_name.find('/') != std::string::npos) {
        // If not a base name, try using this path directly
        config_file_path = config_file_name;
    } else {
        // Check if the file is in the current directory
        if (std::filesystem::exists(config_file_name)) {
            config_file_path = config_file_name;
        } else {
            // Otherwise, prepend the config directory from the environment variable
            config_file_path = std::string(std::getenv("MU_RECO_PATH")) + "/config/" + config_file_name;
        }
    }
    if (!std::filesystem::exists(config_file_path)) {
        std::cerr << "Config file doesn't exist. Could not find " << config_file_path << std::endl;
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
    if (argc > 3)
    {
        output_file_name = argv[3];
    }
    else{
        output_file_name = input_file_name.substr(input_file_name.find_last_of("/\\") + 1);
        output_file_name = output_file_name.substr(0, output_file_name.find_first_of('.')) + ".root";
    }
    std::cout << "-> main: Output file: " << output_file_name << std::endl;

    // End of parsing command line arguments
    // -----------------------------------------------------------------------------------------------


    // -----------------------------------------------------------------------------------------------
    // Set up the various managers etc.

    // Create configuration holder
    std::shared_ptr<reco::ConfigHolder> configHolder = std::make_shared<reco::ConfigHolder>();
    configHolder->LoadFromFile(config_file_path);
    configHolder->SetRunSubrun(run, subrun);

    // Create the output manager
    reco::OutputManager* outputManager = new reco::WFD5OutputManager(output_file_name);
    outputManager->Configure(configHolder);

    // Create the midas event unpacker
    unpackers::EventUnpacker* eventUnpacker = new unpackers::WFD5EventUnpacker();
    // Determine max number of midas events to process
    int maxMidasEvents = -1;
    if (configHolder->GetConfig().contains("Unpacker")) {
        const auto& unpackerConfig = configHolder->GetConfig().at("Unpacker");
        if (unpackerConfig.contains("max_midas_events") && unpackerConfig["max_midas_events"].is_number_integer()) {
            maxMidasEvents = unpackerConfig.value("max_midas_events", -1);
        }
    } else {
        throw std::runtime_error("Missing 'Unpacker' config section with 'max_midas_events'");
    }
    std::cout << "-> main: Max number of events to process: " << maxMidasEvents << std::endl;

    // Set verbosity for unpacker
    int unpackerVerbosity = 0; // Default verbosity
    if (configHolder->GetConfig().contains("Unpacker")) {
        const auto& unpackerConfig = configHolder->GetConfig().at("Unpacker");
        if (unpackerConfig.contains("verbosity") && unpackerConfig["verbosity"].is_number_integer()) {
            unpackerVerbosity = unpackerConfig.value("verbosity", 0);
        }
    } else {
        std::cout << "-> main: Missing 'Unpacker' config section with 'verbosity'. Using default." << std::endl;
    }
    std::cout << "-> main: Setting unpacker verbosity to " << unpackerVerbosity << std::endl;
    LoggerHolder::getInstance().SetVerbosity(unpackerVerbosity);
    
    // Create the EventStore to hold all the data products during processing
    reco::EventStore eventStore;
    eventStore.SetRunSubrun(run, subrun);

    // Create the service manager
    std::shared_ptr<reco::ServiceManager> serviceManager = std::make_shared<reco::ServiceManager>();
    serviceManager->Configure(configHolder, eventStore);

    // Create the reco manager
    reco::RecoManager recoManager;
    recoManager.Configure(configHolder, *serviceManager, eventStore);

    // Create the Midas reader
    TMReaderInterface *mReader = TMNewReader(input_file_name.c_str());

    // -----------------------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------------------
    // Prep for the midas event loop
    int nProcessedMidasEvents = 0;
    int nProcessedEvents = 0;
    auto unpackerStart = std::chrono::high_resolution_clock::now();
    auto unpackerStop = std::chrono::high_resolution_clock::now();
    double unpackerDuration = 0;
    auto writeStart = std::chrono::high_resolution_clock::now();
    auto writeStop = std::chrono::high_resolution_clock::now();
    double writeDuration = 0;

    // loop over the events
    while (true) {

        if ((nProcessedMidasEvents >= maxMidasEvents) && (maxMidasEvents != -1)) {
            // Reached the max number of events to process
            break;
        }

        TMEvent *thisEvent = TMReadEvent(mReader);
        //if (!thisEvent || nProcessedEvents > 100 )
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
            nProcessedMidasEvents++;
            // std::cout << "Processing midas event " << nProcessedMidasEvents << std::endl;
            
            // Unpack the event; this is done in a loop in case there are multiple "trigger" events in the midas event
            unpackers::unpackingStatus status = unpackers::unpackingStatus::SuccessMore;
   
            while (status == unpackers::unpackingStatus::SuccessMore) {

                unpackerStart = std::chrono::high_resolution_clock::now();
                status = eventUnpacker->UnpackEvent(thisEvent);
                
                if (status != unpackers::unpackingStatus::SuccessMore) {
                    continue;
                }
                nProcessedEvents++;
                
                // Put the unpacked data into the event store
                eventStore.clear();  // clear previous event's data
                eventStore.put<dataProducts::WFD5Header>("unpacker","WFD5HeaderCollection", eventUnpacker->GetNextPtrCollection<dataProducts::WFD5Header>("WFD5HeaderCollection"));
                eventStore.put<dataProducts::WFD5ChannelHeader>("unpacker","WFD5ChannelHeaderCollection", eventUnpacker->GetNextPtrCollection<dataProducts::WFD5ChannelHeader>("WFD5ChannelHeaderCollection"));
                eventStore.put<dataProducts::WFD5WaveformHeader>("unpacker","WFD5WaveformHeaderCollection", eventUnpacker->GetNextPtrCollection<dataProducts::WFD5WaveformHeader>("WFD5WaveformHeaderCollection"));
                eventStore.put<dataProducts::WFD5Waveform>("unpacker","WFD5WaveformCollection", eventUnpacker->GetNextPtrCollection<dataProducts::WFD5Waveform>("WFD5WaveformCollection"));

                // Run reconstruction stages
                try {
                    recoManager.Run(eventStore, *serviceManager);
                } catch (const std::exception& e) {
                    std::cerr << "Error during reconstruction: " << e.what() << std::endl;
                    return 1;
                }

                // Fill the output tree with the event data
                writeStart = std::chrono::high_resolution_clock::now();
                outputManager->FillEvent(eventStore);
                writeStop = std::chrono::high_resolution_clock::now();
                writeDuration += std::chrono::duration<double>(writeStop - writeStart).count();
            }

            // Clean up the event now that we are done with it
            delete thisEvent;
            continue;
            
        } // end if event id = 1

    } // end loop over events

    writeStart = std::chrono::high_resolution_clock::now();
    outputManager->WriteHistograms(eventStore);
    outputManager->WriteSplines(eventStore);
    writeStop = std::chrono::high_resolution_clock::now();
    writeDuration += std::chrono::duration<double>(writeStop - writeStart).count();

    // Clean up
    delete eventUnpacker;
    delete outputManager;
    delete mReader;

    std::cout << "-> main: Processed " << nProcessedMidasEvents << " midas events" << std::endl;
    std::cout << "-> main: Processed " << nProcessedEvents << " \"real\" events" << std::endl;
    std::cout << "-> main: All done!" << std::endl;

    std::cout << "-> main: Unpacking took "
        << std::setprecision(4) << unpackerDuration << "/" << nProcessedEvents
        << " = "
        << (unpackerDuration / nProcessedEvents) << " seconds/event"
        << std::endl;

    std::cout << "-> main: Filling tree / writing took "
        << std::setprecision(4) << writeDuration << "/" << nProcessedEvents
        << " = "
        << (writeDuration / nProcessedEvents) << " seconds/event"
        << std::endl;

    // Print out end of job info for each service
    std::cout << "-> main: Services summary:\n";
    serviceManager->EndOfJobPrint();
    
    // Stop the clock!
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << std::setprecision(5) << "-> main: Total elapsed time: " << elapsed.count() << " seconds\n";

    return 0;
}
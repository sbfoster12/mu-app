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

// Custom - Unpackers
#include "unpackers/common/Logger.hh"
#include "unpackers/nalu/NaluEventUnpacker.hh"

// Custom - Data Products
#include <data_products/nalu/NaluEventHeader.hh>
#include <data_products/nalu/NaluPacketHeader.hh>
#include <data_products/nalu/NaluWaveform.hh>
#include <data_products/nalu/NaluPacketFooter.hh>
#include <data_products/nalu/NaluEventFooter.hh>
#include <data_products/nalu/NaluTime.hh>
#include <data_products/nalu/NaluODB.hh>

#include <string>
#include <sstream>
// #include <nlohmann/json.hpp>

using unpackers::LoggerHolder;

int main(int argc, char *argv[])
{
    
    // -----------------------------------------------------------------------------------------------
    // Parse command line arguments

    // We need three arguments: program & file name & verbosity
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " input_file_name verbosity" << std::endl;
        return 1;
    }

    // input file name
    std::string input_file_name = argv[1];

    int verbosity = std::atoi(argv[2]);

    // Check if input file exists
    if (!std::filesystem::exists(input_file_name)) {
        std::cerr << "Input file doesn't exist. Could not find " << input_file_name << std::endl;
        return 1;
    }

    // output file name
    std::string output_file_name;
    output_file_name = input_file_name.substr(input_file_name.find_last_of("/\\") + 1);
    output_file_name = output_file_name.substr(0, output_file_name.find_last_of('.')) + ".root";
    std::cout << "Output file: " << output_file_name << std::endl;

    // Set verbosity for unpacker
    LoggerHolder::getInstance().SetVerbosity(verbosity);

    // End of parsing command line arguments
    // -----------------------------------------------------------------------------------------------

    // create the output file structure
    TFile *outfile = new TFile(output_file_name.c_str(),"RECREATE");

    // without these next lines, time to process 1 file on my local machine: 5.500s, 62MB
    // outfile->SetCompressionLevel(0); // much faster, but the file size doubles (62->137 MB), 2.936s
    outfile->SetCompressionAlgorithm(4); // LZ4. 40-50% faster, but slightly larger file sizes. 3.292s, 91MB
    TTree *tree = new TTree("tree", "tree");

    dataProducts::NaluEventHeaderCollection nalu_event_headers;
    tree->Branch("nalu_event_headers", &nalu_event_headers);

    dataProducts::NaluPacketHeaderCollection nalu_packet_headers;
    tree->Branch("nalu_packet_headers", &nalu_packet_headers);

    dataProducts::NaluWaveformCollection nalu_waveforms;
    tree->Branch("nalu_waveforms", &nalu_waveforms);

    dataProducts::NaluPacketFooterCollection nalu_packet_footers;
    tree->Branch("nalu_packet_footers", &nalu_packet_footers);

    dataProducts::NaluEventFooterCollection nalu_event_footers;
    tree->Branch("nalu_event_footers", &nalu_event_footers);

    dataProducts::NaluTimeCollection nalu_times;
    tree->Branch("nalu_times", &nalu_times);

    dataProducts::NaluODB nalu_odb;

    // -----------------------------------------------------------------------------------------------

    // Set up an event unpacker object
    // This object contains the methods for
    // doing the unpacking of a TMEvent
    auto eventUnpacker = new unpackers::NaluEventUnpacker();

    // We need to get a midas event
    TMReaderInterface *mReader = TMNewReader(input_file_name.c_str());

    int nTotalMidasEvents = 0;
    int nSkippedMidasEvents= 0;

    // loop over the events
    while (true)
    {
        TMEvent *thisEvent = TMReadEvent(mReader);
        //if (!thisEvent || nTotalMidasEvents > 100 )
        if (!thisEvent || nTotalMidasEvents > 100 )
        {
            // Reached end of the file. Clean up and break
            delete thisEvent;
            break;
        }

        if (thisEvent->serial_number % 100 == 0) {
            std::cout << "event_id: " << thisEvent->event_id << ", serial number: " << thisEvent->serial_number << std::endl;
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
                nalu_odb = dataProducts::NaluODB(odb_dump);
                outfile->WriteObject(&nalu_odb, "nalu_odb");
            }
            delete thisEvent;
            continue;
        }

        thisEvent->FindAllBanks();
        // thisEvent->PrintBanks();
        // auto bank = thisEvent->FindBank("AD%0");
        // std::cout << thisEvent->BankToString(bank) << std::endl;

        // only unpack events with id 1
        if (event_id > 0) {
            nTotalMidasEvents++;
            //std::cout << "Processing event " << nTotalMidasEvents << std::endl;
            // Unpack the event
            // This will fill the dataproduct collections
            auto status = eventUnpacker->UnpackEvent(thisEvent);
        
            // std::cout << "status: " << status << std::endl;
            // Only proceed if the status = 0
            if (status != 0) {
                delete thisEvent;
                continue;
            }
            // break;

            nalu_event_headers = eventUnpacker->GetCollection<dataProducts::NaluEventHeader>("NaluEventHeaderCollection");
            nalu_packet_headers = eventUnpacker->GetCollection<dataProducts::NaluPacketHeader>("NaluPacketHeaderCollection");
            nalu_waveforms = eventUnpacker->GetCollection<dataProducts::NaluWaveform>("NaluWaveformCollection");
            nalu_packet_footers = eventUnpacker->GetCollection<dataProducts::NaluPacketFooter>("NaluPacketFooterCollection");
            nalu_event_footers = eventUnpacker->GetCollection<dataProducts::NaluEventFooter>("NaluEventFooterCollection");
            nalu_times = eventUnpacker->GetCollection<dataProducts::NaluTime>("NaluTimeCollection");

            tree->Fill();
            nalu_event_headers.clear();
            nalu_packet_headers.clear();
            nalu_waveforms.clear();
            nalu_packet_footers.clear();
            nalu_event_footers.clear();
            nalu_times.clear();

        } // end if event id = 1

    } // end loop over events

    // clean up
    delete eventUnpacker;
    delete mReader;

    tree->Write();
    outfile->Close();

    std::cout << "Skipped " << nSkippedMidasEvents << "/" << nTotalMidasEvents << " midas events" << std::endl;

    std::cout << "All done!" << std::endl;
    return 0;
}

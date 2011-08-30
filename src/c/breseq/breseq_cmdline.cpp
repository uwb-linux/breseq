 /*****************************************************************************

AUTHORS

  Jeffrey E. Barrick <jeffrey.e.barrick@gmail.com>
  David B. Knoester

LICENSE AND COPYRIGHT

  Copyright (c) 2008-2010 Michigan State University
  Copyright (c) 2011 The University of Texas at Austin

  breseq is free software; you can redistribute it and/or modify it under the  
  terms the GNU General Public License as published by the Free Software 
  Foundation; either version 1, or (at your option) any later version.

*****************************************************************************/

#include <iostream>
#include <string>
#include <vector>

#include "breseq/anyoption.h"
#include "breseq/alignment_output.h"
#include "breseq/annotated_sequence.h"
#include "breseq/calculate_trims.h"
#include "breseq/candidate_junctions.h"
#include "breseq/contingency_loci.h"
#include "breseq/coverage_distribution.h"
#include "breseq/error_count.h"
#include "breseq/fastq.h"
#include "breseq/genome_diff.h"
#include "breseq/identify_mutations.h"
#include "breseq/resolve_alignments.h"
#include "breseq/settings.h"
#include "breseq/tabulate_coverage.h"
#include "breseq/contingency_loci.h"
#include "breseq/mutation_predictor.h"
#include "breseq/output.h"


using namespace breseq;
using namespace std;


/*! Analyze FASTQ
 
 Extract information about reads in a FASTQ file.
 
 */

int do_analyze_fastq(int argc, char* argv[]) {
  
	// setup and parse configuration options:
	AnyOption options("Usage: breseq ANALYZE_FASTQ --input input.fastq --convert converted.fastq");
	options
		("help,h", "produce this help message", TAKES_NO_ARGUMENT)
		("input,i", "input FASTQ file")
		("convert,c", "converted FASTQ file (created only if necessary)")
		//("output,o", "out to files") // outputs to STDOUT for now
	.processCommandArgs(argc, argv);
	
	// make sure that the config options are good:
	if(options.count("help")
		 || !options.count("input")
     || !options.count("convert")
		 ) {
		options.printUsage();
		return -1;
	}                       
  
	try {
    
    AnalyzeFastq af = analyze_fastq(options["input"], options["convert"]);
    
    // Output information to stdout
    cout << "max_read_length "       << af.max_read_length         << endl;
    cout << "num_reads "             << af.num_reads               << endl;
    cout << "min_quality_score "     << (int)af.min_quality_score  << endl;
    cout << "max_quality_score "     << (int)af.max_quality_score  << endl;
    cout << "num_bases "             << af.num_bases               << endl;
    cout << "original_qual_format "  << af.original_qual_format    << endl;
    cout << "qual_format "           << af.quality_format          << endl;
    cout << "converted_fastq_name "  << af.converted_fastq_name    << endl;
    
  } catch(...) {
		// failed; 
		return -1;
	}
  
  return 0;
}

/*! Convert Genbank
 
 Create a tab-delimited file of information about genes and a
 FASTA sequence file from an input GenBank file.
 */

// Helper function
void convert_genbank(const vector<string>& in, const string& fasta, const string& ft, const string& gff3 ) {
  
  cReferenceSequences refseqs;
  
  // Load the GenBank file
  LoadGenBankFile(refseqs, in);
  
  // Output sequence
  if (fasta != "") refseqs.WriteFASTA(fasta);
  
  // Output feature table
  if (ft != "") refseqs.WriteFeatureTable(ft);
     
  if (gff3 != "" ) refseqs.WriteGFF( gff3 );
}

void convert_bull_form(const vector<string>& in, const string& fasta, const string& ft, const string& gff3 ) {
  cReferenceSequences refseqs;
  
  // Load the GenBank file
  LoadBullFile(refseqs, in);
  
  // Output sequence
  if (fasta != "") refseqs.WriteFASTA(fasta);
  
  // Output feature table
  if (ft != "") refseqs.WriteFeatureTable(ft);
  
  if (gff3 != "" ) refseqs.WriteGFF( gff3 );
}


int do_convert_genbank(int argc, char* argv[]) {
	
	// setup and parse configuration options:
	AnyOption options("Usage: breseq CONVERT_GENBANK --input <sequence.gbk> [--fasta <output.fasta> --features <output.tab>]");
	options
		("help,h", "produce this help message", TAKES_NO_ARGUMENT)
		("input,i", "input GenBank flatfile (multiple allowed, comma-separated)")
		("features,g", "output feature table", "")
		("fasta,f", "FASTA file of reference sequences", "")
    ("gff3,v", "GFF file of features", "" )
	.processCommandArgs(argc, argv);
	
	// make sure that the config options are good:
	if(options.count("help")
		 || !options.count("input")
		 || (!options.count("features") && !options.count("fasta"))  
		 ) {
		options.printUsage();
		return -1;
	}
  
	// attempt to calculate error calibrations:
	try {
        
		convert_genbank(  
                    from_string<vector<string> >(options["input"]),
                    options["fasta"],
                    options["features"],
                    options["gff3"]
                    );
  } catch(...) {
		// failed; 
		return -1;
	}
	
	return 0;
}

int do_convert_bull_form(int argc, char* argv[]) {
	
	// setup and parse configuration options:
	AnyOption options("Usage: breseq CONVERT_BULL_FORM --input <bull_form.txt> [--features <output.tab>]");
	options
  ("help,h", "produce this help message", TAKES_NO_ARGUMENT)
  ("input,i", "input bull form flatfile (multiple allowed, comma-separated)")
  ("features,g", "output feature table", "")
  ("fasta,f", "FASTA file of reference sequences", "")
  ("gff3,v", "GFF file of features", "" )
	.processCommandArgs(argc, argv);
	
	// make sure that the config options are good:
	if(options.count("help")
		 || !options.count("input")
		 || (!options.count("features") /*&& !options.count("fasta")*/)  
		 ) {
		options.printUsage();
		return -1;
	}
  
	// attempt to calculate error calibrations:
	try {
    
		convert_bull_form(  
                      from_string<vector<string> >(options["input"]),
                      options["fasta"],
                      options["features"],
                      options["gff3"]
                      );
  } catch(...) {
		// failed; 
		return -1;
	}
	
	return 0;
}


/*! Calculate Trims
 
 Calculate how much to ignore on the end of reads due to ambiguous alignments
 of those bases.
 
 */
int do_calculate_trims(int argc, char* argv[]) {
	
	// setup and parse configuration options:
	AnyOption options("Usage: breseq CALCULATE_TRIMS --bam <sequences.bam> --fasta <reference.fasta> --error_dir <path> --genome_diff <path> --output <path> --readfile <filename> --coverage_dir <dirname> [--minimum-quality-score 3]");
	options
		("help,h", "produce this help message", TAKES_NO_ARGUMENT)
		("fasta,f", "FASTA file of reference sequence")
		("output,o", "output directory")
	.processCommandArgs(argc, argv);
	
	// make sure that the config options are good:
	if(options.count("help")
		 || !options.count("fasta")
		 || !options.count("output")
		 ) {
		options.printUsage();
		return -1;
	}                       
  
	// attempt to calculate error calibrations:
	try {
		calculate_trims(options["fasta"],options["output"]);
  } catch(...) {
		// failed; 
		return -1;
	}
	
	return 0;
}

/*!  Resolve alignments
 
 Compare matches to candidate junctions and matches to reference sequence
 to determine what junctions have support. Also splits mosaic matches.
 
 */
int do_resolve_alignments(int argc, char* argv[]) {
	
	// setup and parse configuration options:
	AnyOption options("Usage: breseq RESOLVE_ALIGNMENTS ... ");
	options
		("help,h", "produce this help message", TAKES_NO_ARGUMENT)
		("no-junction-prediction,p", "whether to predict new junctions", TAKES_NO_ARGUMENT)
// convert to basing everything off the main output path, so we don't have to set so many options
    ("path", "path to breseq output")
		("readfile,r", "FASTQ read files (multiple allowed, comma-separated) ")
		("maximum-read-length,m", "number of flanking bases in candidate junctions")
		("alignment-read-limit", "maximum number of alignments to process. DEFAULT = 0 (OFF).", 0)
    ("junction-cutoff", "coverage cutoffs for different reference sequences")

	.processCommandArgs(argc, argv);

	// make sure that the config options are good:
	if(options.count("help")
     || !options.count("path")
     || !options.count("readfile")
		 || !options.count("maximum-read-length")
		 || (!options.count("no-junction-prediction") && !options.count("junction-cutoff"))
		 ) {
		options.printUsage();
		return -1;
	}                       
  
	try {
    
    Settings settings(options["path"]);

    Summary summary;
    

    cReadFiles rf(from_string<vector<string> >(options["readfile"]));
    
    // Load the reference sequence info
    cReferenceSequences ref_seq_info;
    LoadFeatureIndexedFastaFile(ref_seq_info, settings.reference_features_file_name, settings.reference_fasta_file_name);
    
    // should be one coverage cutoff value for each reference sequence
    vector<double> coverage_cutoffs;
    
    if (!options.count("no-junction-prediction")) {
      coverage_cutoffs = from_string<vector<double> >(options["junction-cutoff"]);
      assert(coverage_cutoffs.size() == ref_seq_info.size());
      
      for (uint32_t i=0; i<ref_seq_info.size(); i++)
      {
        summary.preprocess_coverage[ref_seq_info[i].m_seq_id].junction_accept_score_cutoff = coverage_cutoffs[i];
      }
    }
    
        
    resolve_alignments(
      settings,
      summary,
      ref_seq_info,
      !options.count("no-junction-prediction"),
      rf,
      from_string<uint32_t>(options["maximum-read-length"]),
      from_string<uint32_t>(options["alignment-read-limit"])
    );
    
  } catch(...) {
		// failed; 
    
		return -1;
	}
	
	return 0;
}


/*!  Predict Mutations
 
 Predict mutations from evidence in a genome diff file.
 
 */

int do_predict_mutations(int argc, char* argv[]) {
	
	// setup and parse configuration options:
	AnyOption options("Usage: breseq PREDICT_MUTATIONS ... ");
	options
  ("help,h", "produce this help message", TAKES_NO_ARGUMENT)
  // convert to basing everything off the main output path, so we don't have to set so many options
  ("path", "path to breseq output")
  ("maximum-read-length,m", "number of flanking bases in candidate junctions")
	.processCommandArgs(argc, argv);
  
	// make sure that the config options are good:
	if(options.count("help")
     || !options.count("path")
		 || !options.count("maximum-read-length")
		 ) {
		options.printUsage();
		return -1;
	}                       
  
	try {
    
    Settings settings(options["path"]);
            
    // Load the reference sequence info
    cReferenceSequences ref_seq_info;
    LoadFeatureIndexedFastaFile(ref_seq_info, settings.reference_features_file_name, settings.reference_fasta_file_name);
        
    MutationPredictor mp(ref_seq_info);
    
    genome_diff gd( settings.evidence_genome_diff_file_name );
    
    mp.predict(
               settings,
               gd,
               from_string<uint32_t>(options["maximum-read-length"])
               );
    
    gd.write(settings.final_genome_diff_file_name);
    
  } catch(...) {
		// failed; 
    
		return -1;
	}
	
	return 0;
}




/*! Error Count
 
Calculate error calibrations from FASTA and BAM reference files.
 
 */
int do_error_count(int argc, char* argv[]) {
	  
	// setup and parse configuration options:
	AnyOption options("Usage: breseq ERROR_COUNT --bam <sequences.bam> --fasta <reference.fasta> --output <path> --readfile <filename> [--coverage] [--errors] [--minimum-quality-score 3]");
	options
		("help,h", "produce this help message", TAKES_NO_ARGUMENT)
		("bam,b", "bam file containing sequences to be aligned")
		("fasta,f", "FASTA file of reference sequence")
		("output,o", "output directory")
		("readfile,r", "name of readfile (no extension). may occur multiple times")
		("coverage", "generate unique coverage distribution output", TAKES_NO_ARGUMENT)
		("errors", "generate unique error count output", TAKES_NO_ARGUMENT)
    ("covariates", "covariates for error model", "")
    ("minimum-quality-score", "ignore base quality scores lower than this", 0)
	.processCommandArgs(argc, argv);
  
	// make sure that the config options are good:
	if(options.count("help")
		 || !options.count("bam")
		 || !options.count("fasta")
		 || !options.count("output")
		 || !options.count("readfile")
		 || (!options.count("coverage") && !options.count("errors")) ) {
		options.printUsage();
		return -1;
	}
	
	// attempt to calculate error calibrations:
	try {
		breseq::error_count(options["bam"],
												options["fasta"],
												options["output"],
												split(options["readfile"], "\n"),
												options.count("coverage"),
                        options.count("errors"),
                        from_string<uint32_t>(options["minimum-quality-score"]),
                        options["covariates"]
                        );
	} catch(...) {
		// failed; 
    std::cout << "<<<Failed>>>" << std::endl;
		return -1;
	}
  
	return 0;
}


/*! Identify mutations
 
 Identify read-alignment and missing coverage mutations by going through
 the pileup of reads at each position in the reference genome.
 
 */
int do_identify_mutations(int argc, char* argv[]) {
	
	// setup and parse configuration options:
	AnyOption options("Usage: breseq IDENTIFY_MUTATIONS --bam <sequences.bam> --fasta <reference.fasta> --error_dir <path> --genome_diff <path> --output <path> --readfile <filename> --coverage_dir <dirname>");
	options
		("help,h", "produce this help message", TAKES_NO_ARGUMENT)
		("bam,b", "bam file containing sequences to be aligned")
		("fasta,f", "FASTA file of reference sequence")
		("readfile,r", "names of readfile (no extension)")
		("error_dir,e", "Directory containing error rates files")
		("error_table", "Error rates files", "")
		("genome_diff,g", "Genome diff file")
		("output,o", "output directory")
		("coverage_dir", "directory for coverage files", "")
		("mutation_cutoff,c", "mutation cutoff (log10 e-value)", 2.0L)
		("deletion_propagation_cutoff,u", "number after which to cutoff deletions")
		("minimum_quality_score", "ignore base quality scores lower than this", 0)
		("predict_deletions,d", "whether to predict deletions", TAKES_NO_ARGUMENT)
		("predict_polymorphisms,p", "whether to predict polymorphisms", TAKES_NO_ARGUMENT)
		("polymorphism_cutoff", "polymorphism cutoff (log10 e-value)", 2.0L)
		("polymorphism_frequency_cutoff", "ignore polymorphism predictions below this frequency", 0.0L)
		("per_position_file", "print out verbose per position file", TAKES_NO_ARGUMENT)
	.processCommandArgs(argc, argv);

	// make sure that the config options are good:
	if(options.count("help")
		 || !options.count("bam")
		 || !options.count("fasta")
		 || !options.count("error_dir")
		 || !options.count("genome_diff")
		 || !options.count("output")
		 || !options.count("readfile")
		 || !options.count("coverage_dir")
		 || !options.count("deletion_propagation_cutoff") 
     
		 ) {
		options.printUsage();
		return -1;
	}                       
  
	// attempt to calculate error calibrations:
	try {
		identify_mutations(
                         options["bam"],
                         options["fasta"],
                         options["error_dir"],
                         options["genome_diff"],
                         options["output"],
                         split(options["readfile"], "\n"),
                         options["coverage_dir"],
                         from_string<vector<double> >(options["deletion_propagation_cutoff"]),
                         from_string<double>(options["mutation_cutoff"]),
                         options.count("predict_deletions"),
                         options.count("predict_polymorphisms"),
                         from_string<int>(options["minimum_quality_score"]),
                         from_string<double>(options["polymorphism_cutoff"]),
                         from_string<double>(options["polymorphism_frequency_cutoff"]),
                         options["error_table"],
                         options.count("per_position_file")
                         );
	} catch(...) {
		// failed; 
		return -1;
	}
	
	return 0;
}

/*! Contingency Loci
 
 Analyze lengths of homopolymer repeats in mixed samples.
 
 */
int do_contingency_loci(int argc, char* argv[]) {
	
	// setup and parse configuration options:
	AnyOption options("Usage: breseq CONTINGENCY_LOCI --bam <sequences.bam> --fasta <reference.fasta> --output <path> --loci <loci.txt> --strict");
	options
  ("help,h", "produce this help message", TAKES_NO_ARGUMENT)
  ("bam,b", "bam file containing sequences to be aligned")
  ("fasta,f", "FASTA file of reference sequence")
  ("output,o", "output file")
  ("loci,l", "Contingency loci coordinates" )
  ("strict,s", "exclude non-perfect matches in surrounding 5 bases", TAKES_NO_ARGUMENT)
	.processCommandArgs(argc, argv);
  
	// make sure that the config options are good:
	if(options.count("help")
		 || !options.count("bam")
		 || !options.count("fasta")
		 || !options.count("output")
		 ) {
		options.printUsage();
		return -1;
	}                       
  
	// attempt to calculate error calibrations:
	try {
    
    if( !options.count("loci") ){
      analyze_contingency_loci(
                               options["bam"],
                               options["fasta"],
                               options["output"],
                               NULL,
                               options.count("strict")
                               );
    }
		else{ analyze_contingency_loci(
                       options["bam"],
                       options["fasta"],
                       options["output"],
                      options["loci"],
                        options.count("strict")
                       );
    }
	} catch(...) {
		// failed; 
		return -1;
	}
	
	return 0;
}



/* Candidate Junctions
 
 Perform an analysis of aligned reads to predict the best possible new candidate junctions.
 
 */

int do_preprocess_alignments(int argc, char* argv[]) {

	// setup and parse configuration options:
	AnyOption options("Usage: breseq_utils PREPROCESS_ALIGNMENTS --fasta=reference.fasta --sam=reference.sam --output=output.fasta");
	options
		("help,h", "produce this help message", TAKES_NO_ARGUMENT)
		("data-path", "path of data")
		("reference-alignment-path", "path where input alignment")
		("candidate-junction-path", "path where candidate junction files will be created")
		("read-file,r", "FASTQ read files (multiple allowed, comma-separated) ")

		("candidate-junction-score-method", "scoring method", "POS_HASH")
		("min-indel-split-length", "split indels this long in matches", 3)
		("max-read-mismatches", "ignore reads with more than this number of mismatches", int32_t(-1))
		("require-complete-match", "require the complete read to match (both end bases", TAKES_NO_ARGUMENT)
		("required-match-length", "require this length of sequence -- on the read -- to match", static_cast<uint32_t>(28))
		("candidate-junction-read-limit", "limit handled reads to this many", -1)
    .processCommandArgs(argc, argv);

	// make sure that the config options are good:
	if(options.count("help")
		 || !options.count("data-path")
     || !options.count("reference-alignment-path")
     || !options.count("candidate-junction-path")
     || !options.count("read-file")
		 ) {
		options.printUsage();
		return -1;
	}

	try {
    
  Summary summary;
    
  // Set the things we need...
	Settings settings;
     
  settings.read_files.Init(from_string<vector<string> >(options["read-file"]));
 
	settings.candidate_junction_fasta_file_name = options["candidate-junction-path"];
  settings.candidate_junction_fasta_file_name += "/candidate_junctions.fasta";
	settings.candidate_junction_faidx_file_name = settings.candidate_junction_fasta_file_name + ".fai";
    
	settings.candidate_junction_sam_file_name = options["candidate-junction-path"];
  settings.candidate_junction_sam_file_name += "/#.candidate_junction.sam";
    
	settings.candidate_junction_score_method = options["candidate-junction-score-method"];
    
	settings.preprocess_junction_split_sam_file_name = options["candidate-junction-path"];
  settings.preprocess_junction_split_sam_file_name += "/#.split.sam";
    
	settings.preprocess_junction_best_sam_file_name = options["candidate-junction-path"];
  settings.preprocess_junction_best_sam_file_name += "/best.sam";
    
	settings.reference_fasta_file_name = options["data-path"] + "/reference.fasta";
  settings.reference_faidx_file_name = settings.reference_fasta_file_name + ".fai";
    
	settings.reference_sam_file_name = options["reference-alignment-path"];
  settings.reference_sam_file_name += "/#.reference.sam";
    
  settings.max_read_mismatches = from_string<int32_t>(options["max-read-mismatches"]);
  settings.require_complete_match = options.count("require-complete-match");
	settings.candidate_junction_read_limit = from_string<int32_t>(options["candidate-junction-read-limit"]);
	settings.required_match_length = from_string<int32_t>(options["required-match-length"]);
  settings.preprocess_junction_min_indel_split_length = from_string<int32_t>(options["min-indel-split-length"]);
 
	cReferenceSequences ref_seqs;
	breseq::LoadFeatureIndexedFastaFile(ref_seqs, "", settings.reference_fasta_file_name);

	CandidateJunctions::preprocess_alignments(settings, summary, ref_seqs);

  } catch(...) {
		// failed;
		return -1;
	}

  return 0;
}

int do_identify_candidate_junctions(int argc, char* argv[]) {

	// setup and parse configuration options:
	AnyOption options("Usage: breseq_utils IDENTIFY_CANDIDATE_JUNCTIONS --fasta=reference.fasta --sam=reference.sam --output=output.fasta");
	options
		("help,h", "produce this help message", TAKES_NO_ARGUMENT)
    ("candidate-junction-path", "path where candidate junction files will be created")
    ("data-path", "path of data")
    ("read-file,r", "FASTQ read files (multiple allowed, comma-separated)")

    ("candidate-junction-read-limit", "limit handled reads to this many", static_cast<unsigned long>(0))
    ("required-both-unique-length-per-side,1",
     "Only count reads where both matches extend this many bases outside of the overlap.", static_cast<unsigned long>(5))
    ("required-one-unique-length-per-side,2",
     "Only count reads where at least one match extends this many bases outside of the overlap.", static_cast<unsigned long>(10))
    ("maximum-inserted-junction-sequence-length,3",
     "Maximum number of bases allowed in the overlapping part of a candidate junction.", static_cast<unsigned long>(20))
    ("required-match-length,4",
     "At least this many bases in the read must match the reference genome for it to count.", static_cast<unsigned long>(28))
    ("required-extra-pair-total-length,5",
     "Each match pair must have at least this many bases not overlapping for it to count.", static_cast<unsigned long>(2))
    ("maximum-read-length", "Length of the longest read.")

    // Defaults should be moved to Settings constructor
    ("maximum-candidate-junctions",
     "Maximum number of candidate junction to create.", static_cast<uint32_t>(5000))
    ("minimum-candidate-junctions",
     "Minimum number of candidate junctions to create.", static_cast<uint32_t>(200))
    // This should be in the summary...
    ("reference-sequence-length",
     "Total length of reference sequences.")  
    ("maximum-candidate-junction-length-factor",
     "Total length of candidate junction sequences must be no more than this times reference sequence length.", static_cast<double>(0.1))    
	.processCommandArgs(argc, argv);
  
  //These options are almost always default values
  //TODO: Supply default values?

	// make sure that the config options are good:
	if(options.count("help")
		 || !options.count("data-path")
     || !options.count("candidate-junction-path")
     || !options.count("read-file")
     || !options.count("maximum-read-length")
     || !options.count("reference-sequence-length")
		 ) {
		options.printUsage();
		return -1;
	}                       
  
	try {
    
    // plain function

    Settings settings;
      
    // File settings
    settings.read_files.Init(from_string<vector<string> >(options["read-file"]));
    settings.preprocess_junction_split_sam_file_name = options["candidate-junction-path"] + "/#.split.sam";
    settings.reference_fasta_file_name = options["data-path"] + "/reference.fasta";     
    settings.candidate_junction_fasta_file_name = options["candidate-junction-path"] + "/candidate_junction.fasta";

    // Other settings
    settings.candidate_junction_read_limit = from_string<int32_t>(options["candidate-junction-read-limit"]);
    settings.required_extra_pair_total_length = from_string<int32_t>(options["required-extra-pair-total-length"]);
    settings.required_match_length  = from_string<int32_t>(options["required-match-length"]);
    settings.maximum_inserted_junction_sequence_length = from_string<int32_t>(options["maximum-inserted-junction-sequence-length"]);
    settings.required_one_unique_length_per_side = from_string<int32_t>(options["required-one-unique-length-per-side"]);
    settings.required_both_unique_length_per_side = from_string<int32_t>(options["required-both-unique-length-per-side"]);
    settings.maximum_read_length = from_string<int32_t>(options["maximum-read-length"]);
    
    settings.maximum_candidate_junctions = from_string<int32_t>(options["maximum-candidate-junctions"]);
    settings.minimum_candidate_junctions = from_string<int32_t>(options["minimum-candidate-junctions"]);
    settings.maximum_candidate_junction_length_factor = from_string<double>(options["maximum-candidate-junction-length-factor"]);

    // We should inherit the summary object from earlier steps
    Summary summary;
    summary.sequence_conversion.total_reference_sequence_length = from_string<int32_t>(options["reference-sequence-length"]);
    
    cReferenceSequences ref_seq_info;
    breseq::LoadFeatureIndexedFastaFile(ref_seq_info, "", options["data-path"] + "/reference.fasta");
        
    CandidateJunctions::identify_candidate_junctions(settings, summary, ref_seq_info);
    
  } catch(...) {
		// failed; 
		return -1;
	}
  
  return 0;
}

/*! Tabulate coverage.
 */
int do_tabulate_coverage(int argc, char* argv[]) {
	
	// setup and parse configuration options:
	AnyOption options("Usage: tabulate_coverage --bam <sequences.bam> --fasta <reference.fasta> --output <coverage.tab> [--downsample <float> --detailed]");
	options
		("help,h", "produce this help message", TAKES_NO_ARGUMENT)
		("bam,b", "bam file containing sequences to be aligned")
		("fasta,f", "FASTA file of reference sequence")
		("output,o", "name of output file")
		("region,r", "region to print (accession:start-end)", "")
		("downsample,d", "Only print information every this many positions", 1)
		("read_start_output,r", "Create additional data file binned by read start bases", "")
		("gc_output,g", "Create additional data file binned by GC content of reads", "")
	.processCommandArgs(argc, argv);

	// make sure that the config options are good:
	if(options.count("help")
		 || !options.count("bam")
		 || !options.count("fasta")
		 || !options.count("output")
     ) {
		options.printUsage();
		return -1;
	}  
  
  
	// attempt to calculate error calibrations:
	try {
		breseq::tabulate_coverage(
                              options["bam"],
                              options["fasta"],
                              options["output"],
                              options["region"],
                              from_string<int>(options["downsample"]),
                              options["read_start_output"],
                              options["gc_output"]
						  );
	} catch(...) {
		// failed; 
		return -1;
	}
	
	return 0;
}


int do_convert_gvf( int argc, char* argv[]){
    AnyOption options("Usage: GD2GVF --gd <genomediff.gd> --output <gvf.gvf>"); options( "gd,i","gd file to convert") ("output,o","name of output file").processCommandArgs( argc,argv);
    
    if( !options.count("gd") || !options.count("output") ){
        options.printUsage(); return -1;
    }
    
    try{
        GDtoGVF( options["gd"], options["output"] );
    } 
    catch(...){ 
        return -1; // failed 
    }
    
    return 0;
}

int do_convert_gd( int argc, char* argv[]){
    AnyOption options("Usage: VCF2GD --vcf <vcf.vcf> --output <gd.gd>"); options( "vcf,i","gd file to convert") ("output,o","name of output file").processCommandArgs( argc,argv);
    
    if( !options.count("vcf") || !options.count("output") ){
        options.printUsage(); return -1;
    }
    
    try{
        VCFtoGD( options["vcf"], options["output"] );
    } 
    catch(...){ 
        return -1; // failed 
    }
    
    return 0;
}

int do_output( int argc, char* argv[]){
  (void)argc;
  (void)argv;
  
  //TESTED
  if (false) { 
    Settings settings;
    cout << output::breseq_header_string(settings);
  }

  //TESTED, needs summary before useful.
  if (false) {
    string file_name = "html_statistics.html";
    Settings settings;
    Summary summary;
    cReferenceSequences ref_seq_info;
    output::html_statistics(file_name, settings, summary, ref_seq_info);
  }
    
  //TESTED, displays properly, but there are some oddities between it and 
  //the orgininal, needs calls like diff_entry["gene"] 
  //to be implemented.
  if(false) {
    string file_name = "html_index.html";
    Settings settings;
    settings.no_evidence = false;
    settings.polymorphism_prediction = false;
    settings.lenski_format=false;
    settings.no_header = false;

    Summary summary;
    cReferenceSequences ref_seq_info;
    ref_seq_info.ReadFASTA("/home/geoff/Dropbox/test/1B4/reference.fasta");
    genome_diff gd;
    gd.read("/home/geoff/Dropbox/test/1B4/output.gd");
    output::html_index(file_name, settings, summary, ref_seq_info, gd);
  }
  
  // html_new_junction_table_string needs work
  if (false) {
    string file_name = "html_marginal_predictions.html";
    Settings settings;
    settings.no_evidence = false;
    settings.polymorphism_prediction = false;
    settings.lenski_format=false;
    settings.no_header = false;

    Summary summary;
    cReferenceSequences ref_seq_info;
    ref_seq_info.ReadFASTA("/home/geoffc/Dropbox/test/1B4/reference.fasta");
    genome_diff gd;
    gd.read("/home/geoff/Dropbox/test/1B4/output.gd");
    output::html_marginal_predictions(file_name, settings, summary, ref_seq_info, gd);
  }

  if (false) {
    Settings settings;
    genome_diff gd;
    gd.read("/home/geoff/Dropbox/test/1B4/output.gd");


        
  }



  return 0;
}

int breseq_default_action(int argc, char* argv[])
{
	///
	/// Get options from the command line
	///
  Summary summary;
	Settings settings(argc, argv);
	settings.check_installed();

	//
	// 01_sequence_conversion 
  // * Convert the input reference GenBank into FASTA for alignment
	//
	create_path(settings.sequence_conversion_path);
	create_path(settings.data_path);

	if (settings.do_step(settings.sequence_conversion_done_file_name, "Read and reference sequence file input"))
	{
		Summary::SequenceConversion s;

		//Check the FASTQ format and collect some information about the input read files at the same time
		cerr << "  Analyzing fastq read files..." << endl;
		uint32_t overall_max_read_length = UNDEFINED_UINT32;
		uint32_t overall_max_qual = 0;

		s.num_reads = 0;
    s.num_bases = 0;
		for (uint32_t i = 0; i < settings.read_files.size(); i++)
		{
			string base_name = settings.read_files[i].m_base_name;
			cerr << "    READ FILE::" << base_name << endl;
			string fastq_file_name = settings.base_name_to_read_file_name(base_name);
			string convert_file_name =  settings.file_name(settings.converted_fastq_file_name, "#", base_name);

			// Parse output
			AnalyzeFastq s_rf = analyze_fastq(fastq_file_name, convert_file_name);
      
			// Save the converted file name -- have to save it in summary because only that
			// is reloaded if we skip this step.
			s.converted_fastq_name[base_name] = s_rf.converted_fastq_name;

			// Record statistics
			if ((overall_max_read_length == UNDEFINED_UINT32) || (s_rf.max_read_length > overall_max_read_length))
				overall_max_read_length = s_rf.max_read_length;
			if ((overall_max_qual == UNDEFINED_UINT32) || (s_rf.max_quality_score > overall_max_qual))
				overall_max_qual = s_rf.max_quality_score;
			s.num_reads += s_rf.num_reads;
			s.num_bases += s_rf.num_bases;

			s.reads[base_name] = s_rf;
		}
		s.avg_read_length = s.num_bases / s.num_reads;
		s.max_read_length = overall_max_read_length;
		s.max_qual = overall_max_qual;
		summary.sequence_conversion = s;

		// C++ version of reference sequence processing
		convert_genbank(
			settings.reference_file_names,
			settings.reference_fasta_file_name,
			settings.reference_features_file_name,
			settings.reference_gff3_file_name
		);

		// create SAM faidx
		string samtools = settings.ctool("samtools");
		string command = samtools + " faidx " + settings.reference_fasta_file_name;
		int exit_code = system(command.c_str());
    
		// calculate trim files
		calculate_trims(settings.reference_fasta_file_name, settings.sequence_conversion_path);

		// store summary information
		summary.sequence_conversion.store(settings.sequence_conversion_summary_file_name);
		settings.done_step(settings.sequence_conversion_done_file_name);
	}

	summary.sequence_conversion.retrieve(settings.sequence_conversion_summary_file_name);
	_assert(summary.sequence_conversion.max_read_length != UNDEFINED_UINT32, "Can't retrieve max read length from file: " + settings.sequence_conversion_summary_file_name);

	//load C++ info
	string reference_features_file_name = settings.reference_features_file_name;
	string reference_fasta_file_name = settings.reference_fasta_file_name;
  
  cReferenceSequences ref_seq_info;
  LoadFeatureIndexedFastaFile(ref_seq_info, settings.reference_features_file_name, settings.reference_fasta_file_name);
  
  // @JEB -- This is a bit of an ugly wart.
	// reload certain information into $settings from $summary
	for (map<string, AnalyzeFastq>::iterator it = summary.sequence_conversion.reads.begin(); it != summary.sequence_conversion.reads.end(); it++)
	{
		string read_file = it->first;
		if (it->second.converted_fastq_name.size() > 0)
			settings.read_files.read_file_to_converted_fastq_file_name_map[read_file] = it->second.converted_fastq_name;
	}
	settings.max_read_length = summary.sequence_conversion.max_read_length;
	settings.max_smalt_diff = (settings.max_read_length / 2); // @JEB unused?
	settings.total_reference_sequence_length = summary.sequence_conversion.total_reference_sequence_length;

	//
  // 02_reference_alignment
	// * Match all reads against the reference genome
	//

	if (settings.do_step(settings.reference_alignment_done_file_name, "Read alignment to reference genome"))
	{
		create_path(settings.reference_alignment_path);

		/// create ssaha2 hash
		string reference_hash_file_name = settings.reference_hash_file_name;
		string reference_fasta_file_name = settings.reference_fasta_file_name;

		if (!settings.smalt)
		{
			string command = "ssaha2Build -rtype solexa -skip 1 -save " + reference_hash_file_name + " " + reference_fasta_file_name;
			int exit_code = system(command.c_str());
		}
		else
		{
			string smalt = settings.ctool("smalt");
			string command = smalt + " index -k 13 -s 1 " + reference_hash_file_name + " " + reference_fasta_file_name;
			int exit_code = system(command.c_str());
		}
		/// ssaha2 align reads to reference sequences
		for (uint32_t i = 0; i < settings.read_files.size(); i++)
		{
			cReadFile read_file = settings.read_files[i];

			//reads are paired -- Needs to be re-implemented with major changes elsewhere. @JEB
			//if (is_defined(read_struct.min_pair_dist) && is_defined(read_struct.max_pair_dist))
			//if (is_defined(read_file.m_paired_end_group))
			//{
				// JEB this is not working currently
			//	breseq_assert(false);
				/*
				die "Paired end mapping is broken.";
				die if (scalar @{$read_struct->{read_fastq_list}} != 2);

				my $fastq_1 = $read_struct->{read_fastq_list}->[0];
				my $fastq_2 = $read_struct->{read_fastq_list}->[1];
				my $min = $read_struct->{min_pair_dist};
				my $max = $read_struct->{max_pair_dist};

				my $reference_sam_file_name = $settings->file_name("reference_sam_file_name", {"//"=>$read_struct->{base_name}});
				Breseq::Shared::system("ssaha2 -disk 2 -save $reference_hash_file_name -kmer 13 -skip 1 -seeds 1 -score 12 -cmatch 9 -ckmer 1 -output sam_soft -outfile $reference_sam_file_name -multi 1 -mthresh 9 -pair $min,$max $fastq_1 $fastq_2");
				*/
			//}
			//else //reads are not paired
			{
				string base_read_file_name = read_file.base_name();
				string read_fastq_file = settings.base_name_to_read_file_name(base_read_file_name);
				string reference_sam_file_name = settings.file_name(settings.reference_sam_file_name, "#", base_read_file_name);

				if (!settings.smalt)
				{
					string command = "ssaha2 -disk 2 -save " + reference_hash_file_name + " -kmer 13 -skip 1 -seeds 1 -score 12 -cmatch 9 -ckmer 1 -output sam_soft -outfile " + reference_sam_file_name + " " + read_fastq_file;
					int exit_code = system(command.c_str());
				}
				else
				{
					string smalt = settings.ctool("smalt");
					string command = smalt + " map -n 2 -d " + to_string(settings.max_smalt_diff) + " -f samsoft -o " + reference_sam_file_name + " " + reference_hash_file_name + " " + read_fastq_file; // -m 12
					int exit_code = system(command.c_str());
				}
			}
		}

		/// Delete the hash files immediately
		if (!settings.keep_all_intermediates)
		{
			remove( (reference_hash_file_name + ".base").c_str() );
			remove( (reference_hash_file_name + ".body").c_str() );
			remove( (reference_hash_file_name + ".head").c_str() );
			remove( (reference_hash_file_name + ".name").c_str() );
			remove( (reference_hash_file_name + ".size").c_str() );
		}

		settings.done_step(settings.reference_alignment_done_file_name);
	}

  //
	// 03_candidate_junctions
	// * Identify candidate junctions from split read alignments
	//
	if (!settings.no_junction_prediction)
	{
		create_path(settings.candidate_junction_path);

		if ((settings.preprocess_junction_min_indel_split_length != UNDEFINED_UINT32) || settings.candidate_junction_score_method == "POS_HASH")
		{
			string preprocess_junction_done_file_name = settings.preprocess_junction_done_file_name;

			if (settings.do_step(settings.preprocess_junction_done_file_name, "Preprocessing alignments for candidate junction identification"))
			{
				CandidateJunctions::preprocess_alignments(settings, summary, ref_seq_info);
				settings.done_step(settings.preprocess_junction_done_file_name);
			}
		}

		if (settings.candidate_junction_score_method == "POS_HASH")
		{
			string coverage_junction_summary_file_name = settings.coverage_junction_summary_file_name;

			if (settings.do_step(settings.coverage_junction_done_file_name, "Preliminary analysis of coverage distribution"))
			{
				string reference_faidx_file_name = settings.reference_faidx_file_name;
				string preprocess_junction_best_sam_file_name = settings.preprocess_junction_best_sam_file_name;
				string coverage_junction_best_bam_file_name = settings.coverage_junction_best_bam_file_name;
				string coverage_junction_best_bam_prefix = settings.coverage_junction_best_bam_prefix;
				string coverage_junction_best_bam_unsorted_file_name = settings.coverage_junction_best_bam_unsorted_file_name;

				string samtools = settings.ctool("samtools");

				string command = samtools + " import " + reference_faidx_file_name + " " + preprocess_junction_best_sam_file_name + " " + coverage_junction_best_bam_unsorted_file_name;
				int exit_code = system(command.c_str());
				command = samtools + " sort " + coverage_junction_best_bam_unsorted_file_name + " " + coverage_junction_best_bam_prefix;
				exit_code = system(command.c_str());
				if (!settings.keep_all_intermediates)
					remove(coverage_junction_best_bam_unsorted_file_name.c_str());
				command = samtools + " index " + coverage_junction_best_bam_file_name;
        exit_code = system(command.c_str());

				// Count errors
				string reference_fasta_file_name = settings.reference_fasta_file_name;
				string reference_bam_file_name = settings.coverage_junction_best_bam_file_name;

				error_count(reference_bam_file_name,
					reference_fasta_file_name,
					settings.candidate_junction_path,
					settings.read_file_names,
					true, // coverage
					false, // errors
					settings.base_quality_cutoff,
					"" //covariates
				);

				string error_rates_summary_file_name = settings.error_rates_summary_file_name;
				CoverageDistribution::analyze_unique_coverage_distributions(settings, summary, ref_seq_info,
					settings.coverage_junction_plot_file_name, settings.coverage_junction_distribution_file_name);

				summary.unique_coverage.store(coverage_junction_summary_file_name);
				settings.done_step(settings.coverage_junction_done_file_name);
			}

			summary.preprocess_coverage.retrieve(coverage_junction_summary_file_name);
		}

		string candidate_junction_summary_file_name = settings.candidate_junction_summary_file_name;
		if (settings.do_step(settings.candidate_junction_done_file_name, "Identifying candidate junctions"))
		{
			cerr << "Identifying candidate junctions..." << endl;

			// Other settings
			settings.maximum_read_length = summary.sequence_conversion.max_read_length;

			// We should inherit the summary object from earlier steps
			summary.sequence_conversion.total_reference_sequence_length = settings.total_reference_sequence_length;

      CandidateJunctions::identify_candidate_junctions(settings, summary, ref_seq_info);

			string samtools = settings.ctool("samtools");
			string filename = settings.candidate_junction_path + "/candidate_junction.fasta";
			string faidx_command = samtools + " faidx " + filename;
			if (!file_empty(filename.c_str()))
				int exit_code = system(faidx_command.c_str());

			summary.candidate_junction.store(candidate_junction_summary_file_name);
			settings.done_step(settings.candidate_junction_done_file_name);
		}
		summary.candidate_junction.retrieve(candidate_junction_summary_file_name);

		//
		// Find matches to new junction candidates
		// sub candidate_junction_alignment {}
		//
		if (settings.do_step(settings.candidate_junction_alignment_done_file_name, "Candidate junction alignment"))
		{
			create_path(settings.candidate_junction_alignment_path);

			/// create ssaha2 hash
			string candidate_junction_hash_file_name = settings.candidate_junction_hash_file_name;
			string candidate_junction_fasta_file_name = settings.candidate_junction_fasta_file_name;

			if (!file_empty(candidate_junction_fasta_file_name.c_str()))
			{
				if (!settings.smalt)
				{
					string command = "ssaha2Build -rtype solexa -skip 1 -save " + candidate_junction_hash_file_name + " " + candidate_junction_fasta_file_name;
					int exit_code = system(command.c_str());
				}
				else
				{
					string smalt = settings.ctool("smalt");
					string command = smalt + " index -k 13 -s 1 " + candidate_junction_hash_file_name + " "+ candidate_junction_fasta_file_name;
					int exit_code = system(command.c_str());
				}
			}

			/// ssaha2 align reads to candidate junction sequences
			for (uint32_t i = 0; i < settings.read_files.size(); i++)
			{
				string base_read_file_name = settings.read_files[i].m_base_name;
				string candidate_junction_sam_file_name = settings.file_name(settings.candidate_junction_sam_file_name, "#", base_read_file_name);

				string read_fastq_file = settings.base_name_to_read_file_name(base_read_file_name);
				string filename = candidate_junction_hash_file_name + ".base";
				if (!settings.smalt && file_exists(filename.c_str()))
				{
					string command = "ssaha2 -disk 2 -save " + candidate_junction_hash_file_name + " -best 1 -rtype solexa -skip 1 -seeds 1 -output sam_soft -outfile " + candidate_junction_sam_file_name + " " + read_fastq_file;
					int exit_code = system(command.c_str());
					// Note: Added -best parameter to try to avoid too many matches to redundant junctions!
				}
				else
				{
					filename = candidate_junction_hash_file_name + ".sma";
					if (file_exists(filename.c_str()))
					{
						string smalt = settings.ctool("smalt");
						string command = smalt + " map -c 0.8 -x -n 2 -d 1 -f samsoft -o " + candidate_junction_sam_file_name + " " + candidate_junction_hash_file_name + " " + read_fastq_file;
						int exit_code = system(command.c_str());
					}
				}
			}

			/// Delete the hash files immediately
			if (!settings.keep_all_intermediates)
			{
				remove((candidate_junction_hash_file_name + ".base").c_str());
				remove((candidate_junction_hash_file_name + ".body").c_str());
				remove((candidate_junction_hash_file_name + ".head").c_str());
				remove((candidate_junction_hash_file_name + ".name").c_str());
				remove((candidate_junction_hash_file_name + ".size").c_str());
			}

			settings.done_step(settings.candidate_junction_alignment_done_file_name);
		}
	}
	
	//
	// Resolve matches to new junction candidates
	//sub alignment_correction {}
	//
	string alignment_correction_summary_file_name = settings.alignment_correction_summary_file_name;
	if (settings.do_step(settings.alignment_correction_done_file_name, "Resolving alignments with candidate junctions"))
	{
		create_path(settings.alignment_correction_path);
    
		// should be one coverage cutoff value for each reference sequence
		vector<double> coverage_cutoffs;
		for (uint32_t i = 0; i < ref_seq_info.size(); i++)
    {
      Coverage f = summary.preprocess_coverage[ref_seq_info[i].m_seq_id];
			coverage_cutoffs.push_back(summary.preprocess_coverage[ref_seq_info[i].m_seq_id].junction_accept_score_cutoff);
		}
    assert(coverage_cutoffs.size() == ref_seq_info.size());

		resolve_alignments(
			settings,
			summary,
			ref_seq_info,
      settings.junction_prediction,
			settings.read_files,
			summary.sequence_conversion.max_read_length,
			settings.alignment_read_limit
		);

		summary.alignment_correction.store(settings.alignment_correction_summary_file_name);
		settings.done_step(settings.alignment_correction_done_file_name);
	}
  
	if (file_exists(alignment_correction_summary_file_name.c_str()))
		summary.alignment_correction.retrieve(settings.alignment_correction_summary_file_name);

	//
	// Create BAM files
	//sub bam_creation {}
	//

	if (settings.do_step(settings.bam_done_file_name, "Creating BAM files"))
	{
		create_path(settings.bam_path);

		string reference_faidx_file_name = settings.reference_faidx_file_name;
		string candidate_junction_faidx_file_name = settings.candidate_junction_faidx_file_name;

		string resolved_junction_sam_file_name = settings.resolved_junction_sam_file_name;
		string junction_bam_unsorted_file_name = settings.junction_bam_unsorted_file_name;
		string junction_bam_prefix = settings.junction_bam_prefix;
		string junction_bam_file_name = settings.junction_bam_file_name;

		string samtools = settings.ctool("samtools");
		string command;

		if (!settings.no_junction_prediction)
		{
			command = samtools + " import " + candidate_junction_faidx_file_name + " " + resolved_junction_sam_file_name + " " + junction_bam_unsorted_file_name;
			_system(command);
			command = samtools + " sort " + junction_bam_unsorted_file_name + " " + junction_bam_prefix;
      _system(command);
			if (!settings.keep_all_intermediates)
				remove(junction_bam_unsorted_file_name.c_str());
			command = samtools + " index " + junction_bam_file_name;
			_system(command);
		}

		string resolved_reference_sam_file_name = settings.resolved_reference_sam_file_name;
		string reference_bam_unsorted_file_name = settings.reference_bam_unsorted_file_name;
		string reference_bam_prefix = settings.reference_bam_prefix;
		string reference_bam_file_name = settings.reference_bam_file_name;

		command = samtools + " import " + reference_faidx_file_name + " " + resolved_reference_sam_file_name + " " + reference_bam_unsorted_file_name;
    _system(command);
		command = samtools + " sort " + reference_bam_unsorted_file_name + " " + reference_bam_prefix;
    _system(command);
		if (!settings.keep_all_intermediates)
			remove(reference_bam_unsorted_file_name.c_str());
		command = samtools + " index " + reference_bam_file_name;
    _system(command);

		settings.done_step(settings.bam_done_file_name);
	}

	//
	//#  Graph paired read outliers (experimental)
	//# sub paired_read_distances {}
	//#
	//# {
	//# 	my @rs = settings.read_structures;
	//#
	//# 	my @min_pair_dist;
	//# 	my @max_pair_dist;
	//#
	//# 	my $paired = 0;
	//#
	//# 	my $i=0;
	//# 	foreach my $rfi (@{settings.{read_file_index_to_struct_index}})
	//# 	{
	//# 		$min_pair_dist[$i] = 0;
	//# 		$max_pair_dist[$i] = 0;
	//#
	//# 		if ($rs[$rfi]->{paired})
	//# 		{
	//# 			$paired = 1;
	//# 			$min_pair_dist[$i] = $rs[$rfi]->{min_pair_dist};
	//# 			$max_pair_dist[$i] = $rs[$rfi]->{max_pair_dist};
	//# 		}
	//# 		$i++;
	//# 	}
	//#
	//# 	my $long_pairs_file_name = settings.file_name("long_pairs_file_name");
	//#
	//# 	if ($paired && (!-e $long_pairs_file_name))
	//# 	{
	//#
	//# 		my $reference_sam_file_name = settings.file_name("resolved_reference_sam_file_name");
	//# 		my $reference_tam = Bio::DB::Tam->open($reference_sam_file_name) or die "Could not open $reference_sam_file_name";
	//#
	//# 		my $reference_faidx_file_name = settings.file_name("reference_faidx_file_name");
	//# 		my $reference_header = $reference_tam->header_read2($reference_faidx_file_name) or throw("Error reading reference fasta index file: $reference_faidx_file_name");
	//# 		my $target_names = $reference_header->target_name;
	//#
	//# 		my $save;
	//# 		my $on_alignment = 0;
	//# 		my $last;
	//#
	//# 		while (1)
	//# 		{
	//# 			$a = Bio::DB::Bam::Alignment->new();
	//# 			my $bytes = $reference_tam->read1($reference_header, $a);
	//# 			last if ($bytes <= 0);
	//#
	//#
	//# 			my $start       = $a->start;
	//# 		    my $end         = $a->end;
	//# 		    my $seqid       = $target_names->[$a->tid];
	//#
	//# 			$on_alignment++;
	//# 			print "$on_alignment\n" if ($on_alignment % 10000 == 0);
	//#
	//# 			//#last if ($on_alignment > 100000);
	//#
	//# 			//#print $a->qname . "" << endl;
	//#
	//# 			if (!$a->unmapped)
	//# 			{
	//# 				my $mate_insert_size = abs($a->isize);
	//# 				my $mate_end = $a->mate_end;
	//# 				my $mate_start = $a->mate_start;
	//# 				my $mate_reversed = 2*$a->mreversed + $a->reversed;
	//# 		 		my $mreversed = $a->mreversed;
	//# 		 		my $reversed = $a->reversed;
	//#
	//# 				my $fastq_file_index = $a->aux_get("X2");
	//# 				//#print "$mate_insert_size $min_pair_dist[$fastq_file_index] $max_pair_dist[$fastq_file_index]" << endl;
	//# 				//#if (($mate_insert_size < $min_pair_dist[$fastq_file_index]) || ($mate_insert_size > $max_pair_dist[$fastq_file_index]))
	//# 				if ((($mate_insert_size >= 400) && ($mate_insert_size < $min_pair_dist[$fastq_file_index])) || ($mate_insert_size > $max_pair_dist[$fastq_file_index]))
	//# 				{
	//# 					//#correct pair
	//#
	//# 					if ($last && ($last->{start} == $mate_start))
	//# 					{
	//# 						$save->{int($start/100)}->{int($mate_start/100)}->{$mate_reversed}++;
	//# 						$save->{int($last->{start}/100)}->{int($last->{mate_start}/100)}->{$last->{mate_reversed}}++;
	//# 						undef $last;
	//# 					}
	//# 					else
	//# 					{
	//# 						($last->{mate_reversed}, $last->{start}, $last->{mate_start}) = ($mate_reversed, $start, $mate_start);
	//# 					}
	//#
	//# 					//#$save->{$mate_reversed}->{int($start/100)}->{int($mate_start/100)}++;
	//# 				    //print $a->qname," aligns to $seqid:$start..$end, $mate_start $mate_reversed ($mreversed $reversed) $mate_insert_size" << endl;
	//# 				}
	//#
	//# 			}
	//# 		}
	//#
	//# 		open LP, ">$long_pairs_file_name" or die;
	//#
	//# 		foreach my $key_1 (sort {$a <=> $b} keys %$save)
	//# 		{
	//# 			foreach my $key_2 (sort {$a <=> $b} keys %{$save->{$key_1}})
	//# 			{
	//# 				foreach my $key_reversed (sort {$a <=> $b} keys %{$save->{$key_1}->{$key_2}})
	//# 				{
	//# 					print LP "$key_1\t$key_2\t$key_reversed\t$save->{$key_1}->{$key_2}->{$key_reversed}" << endl;
	//# 				}
	//# 			}
	//# 		}
	//# 		close LP;
	//# 	}
	//#
	//# 	if ($paired)
	//# 	{
	//# 		open LP, "$long_pairs_file_name" or die;
	//# 		while ($_ = <LP>)
	//# 		{
	//# 			chomp $_;
	//# 			my ($start, $end, $key_reversed);
	//# 		}
	//# 	}
	//# }
	//#

	//
	// Tabulate error counts and coverage distribution at unique only sites
	//sub error_count {}
	//

	if (settings.do_step(settings.error_counts_done_file_name, "Tabulating error counts"))
	{
		create_path(settings.error_calibration_path);

		string reference_fasta_file_name = settings.reference_fasta_file_name;
		string reference_bam_file_name = settings.reference_bam_file_name;

		//my $cbreseq = settings.ctool("cbreseq");

		// deal with distribution or error count keys being undefined...
		string coverage_fn = settings.file_name(settings.unique_only_coverage_distribution_file_name, "@", "");
		string outputdir = dirname(coverage_fn) + "/";
		/*chomp $outputdir; $outputdir .= "/";
		my $readfiles = join(" --readfile ", settings.read_files);
		my $cmdline = "$cbreseq ERROR_COUNT --bam $reference_bam_file_name --fasta $reference_fasta_file_name --output $outputdir --readfile $readfiles --coverage";
		$cmdline .= " --errors";
		$cmdline .= " --minimum-quality-score settings.{base_quality_cutoff}" if (settings.{base_quality_cutoff});*/

	//----->//this line should be calculated
		uint32_t num_read_files = summary.sequence_conversion.reads.size();
		uint32_t num_qual = summary.sequence_conversion.max_qual + 1;
		/*$cmdline .= " --covariates=read_set=$num_read_files,obs_base,ref_base,quality=$num_qual";
	//		$cmdline .= " --covariates=read_set=$num_read_files,obs_base,ref_base,quality=$summary->{sequence_conversion}->{max_qual},base_repeat=5";
		Breseq::Shared::system($cmdline);*/

		error_count(
			reference_bam_file_name, // bam
			reference_fasta_file_name, // fasta
			settings.error_calibration_path, // output
			settings.read_files.base_names(), // readfile
			true, // coverage
			true, // errors
			settings.base_quality_cutoff, // minimum quality score
			"read_set=" + to_string(num_read_files) + ",obs_base,ref_base,quality=" + to_string(num_qual) // covariates
		);

		settings.done_step(settings.error_counts_done_file_name);
	}


	//
	// Calculate error rates
	//sub error_rates {}
	//

	create_path(settings.output_path); //need output for plots
  create_path(settings.output_calibration_path);
	string error_rates_summary_file_name = settings.error_rates_summary_file_name;

	if (settings.do_step(settings.error_rates_done_file_name, "Re-calibrating base error rates"))
	{
		//$summary->{unique_coverage} = {};
		if (!settings.no_deletion_prediction)
			CoverageDistribution::analyze_unique_coverage_distributions(settings, summary, ref_seq_info,
				settings.unique_only_coverage_plot_file_name, settings.unique_only_coverage_distribution_file_name);

		string command;
		for (uint32_t i = 0; i<settings.read_files.size(); i++) {
			string base_name = settings.read_files[i].base_name();
			string error_rates_base_qual_error_prob_file_name = settings.file_name(settings.error_rates_base_qual_error_prob_file_name, "#", base_name);
			string plot_error_rates_r_script_file_name = settings.plot_error_rates_r_script_file_name;
			string plot_error_rates_r_script_log_file_name = settings.file_name(settings.plot_error_rates_r_script_log_file_name, "#", base_name);
			string error_rates_plot_file_name = settings.file_name(settings.error_rates_plot_file_name, "#", base_name);
			command = "R --vanilla in_file=" + error_rates_base_qual_error_prob_file_name + " out_file=" + error_rates_plot_file_name + " < " + plot_error_rates_r_script_file_name + " > " + plot_error_rates_r_script_log_file_name;
			_system(command);
		}

		Storable::store(summary.unique_coverage, error_rates_summary_file_name); //or die "Can"t store data in file $error_rates_summary_file_name!" << endl;
		settings.done_step(settings.error_rates_done_file_name);
	}
	Storable::retrieve(summary.unique_coverage, error_rates_summary_file_name);
	//die "Can"t retrieve data from file $error_rates_summary_file_name!\n" if (!$summary->{unique_coverage});
	//these are determined by the loaded summary information
	settings.unique_coverage = summary.unique_coverage;

	//
	// Make predictions of point mutations, small indels, and large deletions
	//sub mutation_prediction {}
	//

	//my @mutations;
	//my @deletions;
	//my @unknowns;

	if (!settings.no_mutation_prediction)
	{
		create_path(settings.mutation_identification_path);

		if (settings.do_step(settings.mutation_identification_done_file_name, "Read alignment mutations"))
		{
			//my $error_rates;

			string reference_fasta_file_name = settings.reference_fasta_file_name;
			string reference_bam_file_name = settings.reference_bam_file_name;

			//my $cbreseq = settings.ctool("cbreseq");

			string coverage_fn = settings.file_name(settings.unique_only_coverage_distribution_file_name, "@", "");
			string error_dir = dirname(coverage_fn) + "/";
			string this_predicted_mutation_file_name = settings.file_name(settings.predicted_mutation_file_name, "@", "");
			string output_dir = dirname(this_predicted_mutation_file_name) + "/";
			string ra_mc_genome_diff_file_name = settings.ra_mc_genome_diff_file_name;
			string coverage_tab_file_name = settings.file_name(settings.complete_coverage_text_file_name, "@", "");
			string coverage_dir = dirname(coverage_tab_file_name) + "/";

			// It is important that these are in consistent order with the fasta file!!
			vector<double> deletion_propagation_cutoffs;
			for (uint32_t i = 0; i < ref_seq_info.size(); i++)
				deletion_propagation_cutoffs.push_back(settings.unique_coverage[ref_seq_info[i].m_seq_id].deletion_coverage_propagation_cutoff);

			identify_mutations(
				reference_bam_file_name,
				reference_fasta_file_name,
				error_dir,
				ra_mc_genome_diff_file_name,
				output_dir,
				settings.read_file_names,
				coverage_dir,
				deletion_propagation_cutoffs,
				settings.mutation_log10_e_value_cutoff, // mutation_cutoff
				!settings.no_deletion_prediction,
				settings.polymorphism_prediction,
				settings.base_quality_cutoff, // minimum_quality_score
				settings.polymorphism_log10_e_value_cutoff, // polymorphism_cutoff
				settings.polymorphism_frequency_cutoff, //polymorphism_frequency_cutoff
				error_dir + "/error_rates.tab",
				false //per_position_file
			);

			settings.done_step(settings.mutation_identification_done_file_name);
		}

		string polymorphism_statistics_done_file_name = settings.polymorphism_statistics_done_file_name;
		if (settings.polymorphism_prediction && settings.do_step(settings.polymorphism_statistics_done_file_name, "Polymorphism statistics"))
		{
			ref_seq_info.polymorphism_statistics(settings, summary);
			settings.done_step(settings.polymorphism_statistics_done_file_name);
		}
	}

	//rewire which GenomeDiff we get data from if we have the elaborated polymorphism_statistics version
	 if (settings.polymorphism_prediction)
		settings.ra_mc_genome_diff_file_name = settings.polymorphism_statistics_ra_mc_genome_diff_file_name;

	if (settings.do_step(settings.output_done_file_name, "Output"))
	{
		///
		// Output Genome Diff File
		//sub genome_diff_output {}
		///
		cerr << "Creating merged genome diff evidence file..." << endl;

		// merge all of the evidence GenomeDiff files into one...
		create_path(settings.evidence_path);
		genome_diff jc_gd(settings.jc_genome_diff_file_name);
		genome_diff ra_mc_gd(settings.ra_mc_genome_diff_file_name);
		genome_diff evidence_gd(jc_gd, ra_mc_gd);
		evidence_gd.write(settings.evidence_genome_diff_file_name);


		// predict mutations from evidence in the GenomeDiff
		cerr << "Predicting mutations from evidence..." << endl;

		MutationPredictor mp(ref_seq_info);
		genome_diff mpgd(settings.evidence_genome_diff_file_name);
		mp.predict(settings, mpgd, summary.sequence_conversion.max_read_length, summary.sequence_conversion.avg_read_length);
		mpgd.write(settings.final_genome_diff_file_name);

		genome_diff gd(settings.final_genome_diff_file_name);
		//#unlink $evidence_genome_diff_file_name;

		//
		// Annotate mutations
		//
		cerr << "Annotating mutations..." << endl;
		ref_seq_info.annotate_mutations(gd);

		//
		// Plot coverage of genome and large deletions
		//
		cerr << "Drawing coverage plots..." << endl;
		output::draw_coverage(settings, ref_seq_info, gd);

		//
		// Mark lowest RA evidence items as no-show, or we may be drawing way too many alignments
		//

		vector<string> ra_types = make_list<string>("RA");
		list<counted_ptr<diff_entry> > ra = gd.filter_used_as_evidence(gd.list(ra_types));

    ra.remove_if(diff_entry::frequency_less_than_two_or_no_show());
    


		//@ra = sort { -($a->{quality} <=> $b->{quality}) } @ra;
    ra.sort(diff_entry::by_scores(make_list<string>("quality"))); 

		list<counted_ptr<diff_entry> >::iterator it;

		it = ra.begin();
		for (uint32_t i = 0; i < ra.size(); i++)
		{
			if (i++ >= settings.max_rejected_polymorphisms_to_show)
				(**(it++))["no_show"] = "true";
		}

		// require a certain amount of coverage
		list<counted_ptr<diff_entry> > new_ra = gd.filter_used_as_evidence(gd.list(ra_types));
		for (list<counted_ptr<diff_entry> >::iterator item = new_ra.begin(); item != new_ra.end(); item++)
		{
			vector<string> top_bot = split((**item)["tot_cov"], "/");
			uint32_t top = from_string<int32_t>(top_bot[0]);
			uint32_t bot = from_string<int32_t>(top_bot[1]);
			if (top + bot <= 2) (**item)["no_show"] = "true";
		}
		
    ra.remove_if(diff_entry::coverage_or_no_show_is_true());

		//
		// Mark lowest scoring reject junctions as no-show
		//
		vector<string> jc_types = make_list<string>("JC");
		list<counted_ptr<diff_entry> > jc = gd.filter_used_as_evidence(gd.list(jc_types));
	  
    for (it = jc.begin(); it != jc.end(); it++)
    {
      if (!from_string<bool>((**it)["reject"]))
        ra.erase(it--);
    }
    
    //jc.remove_if(diff_entry::reject_is_not_true());

		//@jc = sort { -($a->{pos_hash_score} <=> $b->{pos_hash_score}) || -($a->{min_overlap_score} <=> $b->{min_overlap_score})  || ($a->{total_reads} <=> $a->{total_reads}) } @jc;
    jc.sort(diff_entry::by_scores(make_list<diff_entry::key_t>("pos_hash_score")("min_overlap_score")("total_reads")));
		it = jc.begin();
		for (uint32_t i = 0; i < jc.size(); i++)
		{
			if (i++ >= settings.max_rejected_junctions_to_show)
				(**(it++))["no_show"] = "true";
		}

		//
		// Create evidence files containing alignments and coverage plots
		//
		if (!settings.no_alignment_generation) {
			output::EvidenceFiles evidence_files;
      evidence_files.htmlOutput(settings, gd);
      
    }

		///
		// HTML output
		///

		cerr << "Creating index HTML table..." << endl;

		output::html_index(settings.index_html_file_name, settings, summary, ref_seq_info, gd);
		output::html_marginal_predictions(settings.marginal_html_file_name, settings, summary, ref_seq_info, gd);
        
		// record the final time and print summary table
		settings.record_end_time("Output");

		output::html_statistics(settings.summary_html_file_name, settings, summary, ref_seq_info);

		settings.done_step(settings.output_done_file_name);
	}
  
  return 0;
}



/*! breseq commands
 
    First argument is a command that should be removed from argv.
    Down the road -- if there is no command, pass to actual breseq pipeline
 
 */
int main(int argc, char* argv[]) {

	// Extract the sub-command argument
	string command;
	char* argv_new[argc];
	int argc_new = argc - 1;

	if (argc > 1) {

		command = argv[1];
		argv_new[0] = argv[0];
		for (int32_t i = 1; i < argc; i++)
			argv_new[i] = argv[i + 1];

	} else {
		cout << "No [command] provided." << endl;
		cout << "Usage: cbreseq [command] options ..." << endl;
		return -1;
	}

	// Pass the command to the proper handler
	command = to_upper(command);
	if (command == "ANALYZE_FASTQ") {
		return do_analyze_fastq(argc_new, argv_new);
	} else if (command == "CALCULATE_TRIMS") {
		return do_calculate_trims(argc_new, argv_new);
	} else if (command == "CONVERT_GENBANK") {
		return do_convert_genbank(argc_new, argv_new);
  } else if (command == "CONVERT_BULL_FORM") {
    return do_convert_bull_form(argc_new, argv_new);
	} else if (command == "CONTINGENCY_LOCI") {
		return do_contingency_loci(argc_new, argv_new);
	} else if (command == "ERROR_COUNT") {
		return do_error_count(argc_new, argv_new);
	} else if (command == "IDENTIFY_MUTATIONS") {
		return do_identify_mutations(argc_new, argv_new);
	} else if (command == "PREPROCESS_ALIGNMENTS") {
		return do_preprocess_alignments(argc_new, argv_new);
	} else if (command == "IDENTIFY_CANDIDATE_JUNCTIONS") {
		return do_identify_candidate_junctions(argc_new, argv_new);
	} else if (command == "TABULATE_COVERAGE") {
		return do_tabulate_coverage(argc_new, argv_new);
	} else if (command == "RESOLVE_ALIGNMENTS") {
		return do_resolve_alignments(argc_new, argv_new);
  } else if (command == "PREDICT_MUTATIONS") {
		return do_predict_mutations(argc_new, argv_new);
	} else if (command == "GD2GVF") {
    return do_convert_gvf(argc_new, argv_new);
  } else if (command == "VCF2GD") {
    return do_convert_gd( argc_new, argv_new);
  } else if (command == "OUTPUT") {
    return do_output(argc_new, argv_new);
  } else {
    // Not a sub-command. Use original argument list.
    return breseq_default_action(argc, argv);
  }

	// Command was not recognized. Should output valid commands.
	cout << "Unrecognized command" << endl;
	return -1;
}

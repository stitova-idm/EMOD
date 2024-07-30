#!/usr/bin/python

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_vector_genetics_support as dvgs


def application(output_folder="output", config_filename="config.json",
                report_name=dtk_sft.sft_output_filename, debug=False):
    dtk_sft.wait_for_done()
    param_obj = dvgs.load_emod_parameters(config_filename=config_filename, debug=debug)
    dvgs.basics_report_file(param_obj=param_obj, output_folder=output_folder, report_name=report_name, debug=debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output, config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

#!/usr/bin/env python
import optparse, sys, os

import build_cfg, casc

parser = optparse.OptionParser( usage = 'Usage: %prog -d wow_install_dir [options] file_path ...')
parser.add_option( '--cdn', dest = 'online', action = 'store_true', help = 'Fetch data from Blizzard CDN [only used for mode=batch/extract]' )
parser.add_option( '-m', '--mode', dest = 'mode', choices = [ 'batch', 'unpack', 'extract' ],
                   help = 'Extraction mode: "batch" for file extraction, "unpack" for BLTE file unpack, "extract" for key or MD5 based file extract from local game client files' )
parser.add_option( '-b', '--dbfile', dest = 'dbfile', type = 'string', default = 'dbfile', 
					help = "A textual file containing a list of file paths to extract [default dbfile, only needed for mode=batch]" )
parser.add_option( '-r', '--root', dest = 'root_file', type = 'string', default = 'root',
                   help = 'Root file path location [default CACHE_DIR/root, only needed if --cdn is not set]' )
parser.add_option( '-e', '--encoding', dest = 'encoding_file', type = 'string', default = 'encoding',
				   help = 'Encoding file path location [default CACHE_DIR/encoding, only needed if --cdn is not set]' )
parser.add_option( '-d', '--datadir', dest = 'data_dir', type = 'string',
				   help = 'World of Warcraft install directory [only needed if --cdn is not set]' )
parser.add_option( '-o', '--output', type = 'string', dest = 'output',
				   help = "Output directory for dbc mode, output file name for unpack mode" )
parser.add_option( '-x', '--cache', type = 'string', dest = 'cache', default = 'cache', help = 'Cache directory [default cache]' )

if __name__ == '__main__':
	(opts, args) = parser.parse_args()
	opts.parser = parser
	
	if not opts.mode and opts.online:
		cdn = casc.CDNIndex(opts)
		cdn.CheckVersion()
		sys.exit(0)
	elif opts.mode == 'batch':
		if not opts.output:
			parser.error("Batch mode requires an output directory for the files")
			sys.exit(1)
					
		fname_db = build_cfg.DBFileList(opts)
		if not fname_db.open():
			sys.exit(1)
		
		blte = casc.BLTEExtract(opts)
		
		if not opts.online:
			build = build_cfg.BuildCfg(opts)
			if not build.open():
				sys.exit(1)

			encoding = casc.CASCEncodingFile(opts, build)
			if not encoding.open():
				sys.exit(1)
			
			index = casc.CASCDataIndex(opts)
			if not index.open():
				sys.exit(1)

			root = casc.CASCRootFile(opts, build, encoding, index)
			if not root.open():
				sys.exit(1)
			
			for file_hash, file_name in fname_db.iteritems():
				extract_data = None
				
				file_md5s = root.GetFileHashMD5(file_hash)
				file_keys = []
				for md5s in file_md5s:
					file_keys = encoding.GetFileKeys(md5s)

					file_locations = []
					for file_key in file_keys:
						file_location = index.GetIndexData(file_key)
						if file_location[0] > -1:
							extract_data = (file_key, md5s, file_name) + file_location
							break
				
				if not extract_data:
					continue
				
				print 'Extracting %s ...' % file_name
			
				if not blte.extract_file(*extract_data):
					sys.exit(1)
		else:
			cdn = casc.CDNIndex(opts)
			if not cdn.open():
				sys.exit(1)
					
			encoding = casc.CASCEncodingFile(opts, cdn)
			if not encoding.open():
				sys.exit(1)
			
			root = casc.CASCRootFile(opts, cdn, encoding, None)
			if not root.open():
				sys.exit(1)
			
			output_path = os.path.join(opts.output, cdn.build())
			for file_hash, file_name in fname_db.iteritems():
				file_md5s = root.GetFileHashMD5(file_hash)
				if not file_md5s:
					continue
				
				if len(file_md5s) > 1:
					print 'Duplicate files found (%d) for %s, selecting first one ...' % (len(file_md5s), file_name)
				
				file_keys = encoding.GetFileKeys(file_md5s[0])
				
				if len(file_keys) == 0:
					continue
				
				if len(file_keys) > 1:
					print 'More than one key found for %s, selecting first one ...' % file_name

				print 'Extracting %s ...' % file_name

				data = cdn.fetch_file(file_keys[0])
				if not data:
					print 'No data for a given key %s' % file_keys[0].encode('hex')
					continue

				blte.extract_buffer_to_file(data, os.path.join(output_path, file_name.replace('\\', '/')))
				
	elif opts.mode == 'unpack':
		blte = casc.BLTEExtract(opts)
		for file in args:
			print 'Extracting %s ...'
			if not blte.extract_file(file):
				sys.exit(1)
	
	elif opts.mode == 'extract':
		build = None
		index = None
		if not opts.online:
			build = build_cfg.BuildCfg(opts)
			if not build.open():
				sys.exit(1)

			index = casc.CASCDataIndex(opts)
			if not index.open():
				sys.exit(1)
		else:
			build = casc.CDNIndex(opts)
			if not build.open():
				sys.exit(1)
		
		encoding = casc.CASCEncodingFile(opts, build)
		if not encoding.open():
			sys.exit(1)
		
		root = casc.CASCRootFile(opts, build, encoding, index)
		if not root.open():
			sys.exit(1)
			
		keys = []
		md5s = None
		if 'key:' in args[0]:
			keys.append(args[0][4:].decode('hex'))
		elif 'md5:' in args[0]:
			md5s = args[0][4:]
			keys = encoding.GetFileKeys(args[0][4:].decode('hex'))
			if len(keys) == 0:
				parser.error('No file found with md5sum %s' % args[0][4:])		
		else:
			file_md5s = root.GetFileMD5(args[0])
			print args[0], len(file_md5s) and file_md5s[0] or 0
			sys.exit(0)
		
		if len(keys) > 1:
			parser.error('Found multiple file keys with %s' % args[0])

		file_location = index.GetIndexData(keys[0])
		if file_location[0] == -1:
			parser.error('No file location found for %s' % args[0])
		
		blte = casc.BLTEExtract(opts)
		
		if not blte.extract_file(keys[0], md5s and md5s.decode('hex') or None, None, *file_location):
			sys.exit(1)

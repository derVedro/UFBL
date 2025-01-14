#include "Common.h"
struct Run {
	unsigned short start_pos;
	unsigned short end_pos;
	unsigned label;
};
struct Runs {
	Run* runs;
	unsigned height;
	unsigned width;
	Runs(unsigned _height, unsigned _width) : height(_height), width(_width) {
		runs = new Run[height * (width / 2 + 2) + 1];
	};
	~Runs() {
		delete[] runs;
	}
};
inline void CCL_RBTS8_X86_FindRuns(const unsigned int* pdata, int height, int width, Run* runs, UFPC& labelsolver) {
	Run* runs_up = runs;

	for (int i = 0;; runs++) {
		//find starting position
		for (;; i++) {
			if (i >= width) {
				runs->start_pos = (short)0xFFFF;
				runs->end_pos = (short)0xFFFF;
				runs++;
				goto out;
			}
			if (pdata[i]) break;
		}
		runs->start_pos = short(i), i++;

		//find ending position
		for (;; i++) {
			if (i == width || !pdata[i]) break;
		}
		runs->end_pos = short(i), i++;
		runs->label = labelsolver.NewLabel();
	}
out:

	//process runs in the rests
	for (size_t row = 1; row < (size_t)height; row++) {
		Run* runs_save = runs;
		pdata += width;
		for (int i = 0;; runs++) {
			//find starting position
			for (;; i++) {
				if (i >= width) {
					runs->start_pos = (short)0xFFFF;
					runs->end_pos = (short)0xFFFF;
					runs++;
					goto out2;
				}
				if (pdata[i]) break;
			}
			unsigned short start_pos = short(i);
			i++;

			//find ending position
			for (;; i++) {
				if (i == width || !pdata[i]) break;
			}
			unsigned short end_pos = short(i);
			i++;

			//Skip upper runs end before this run starts
			for (; runs_up->end_pos < start_pos; runs_up++);

			//No upper run meets this
			if (runs_up->start_pos > end_pos) {
				runs->start_pos = start_pos;
				runs->end_pos = end_pos;
				runs->label = labelsolver.NewLabel();
				continue;
			};

			unsigned label = labelsolver.GetLabel(runs_up->label);

			//Next upper run can not meet this
			if (end_pos <= runs_up->end_pos) {
				runs->start_pos = start_pos;
				runs->end_pos = end_pos;
				runs->label = label;
				continue;
			}

			//Find next upper runs meet this
			runs_up++;
			for (; runs_up->start_pos <= end_pos; runs_up++) {
				unsigned label_other = labelsolver.GetLabel(runs_up->label);
				if (label != label_other) label = labelsolver.Merge(label, label_other);
				if (end_pos <= runs_up->end_pos) break;
			}
			runs->start_pos = start_pos;
			runs->end_pos = end_pos;
			runs->label = label;
		}

	out2:
		runs_up = runs_save;
	}
	labelsolver.Flatten();
}

void run_based_algorithm_8c(const Data& data, Data& data_labels) {
	int height = data.height;
	int width = data.width;

	//find runs
	UFPC labelsolver;
	labelsolver.Alloc((size_t)((height + 1) / 2) * (size_t)((width + 1) / 2) + 1);
	labelsolver.Setup();
	Runs Data_run(height, width);
	CCL_RBTS8_X86_FindRuns(data.raw, height, width, Data_run.runs, labelsolver);

	//generate label data
	Run* runs = Data_run.runs;
	for (int i = 0; i < height; i++) {
		unsigned int* labels = data_labels.data[i];
		for (int j = 0;; runs++) {
			unsigned short start_pos = runs->start_pos;
			if (start_pos == 0xFFFF) {
				for (; j < width; j++) labels[j] = 0;
				runs++;
				break;
			}
			unsigned short end_pos = runs->end_pos;
			unsigned label = labelsolver.GetLabel(runs->label);
			for (; j < start_pos; j++) labels[j] = 0;
			for (; j < end_pos; j++) labels[j] = label;
		}
	}

	labelsolver.Dealloc();
};
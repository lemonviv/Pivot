import numpy as np
import matplotlib.pyplot as plt

def plot_learning_curve(lines, shapes, colors, labels, markers,
                        save_path, title='', logy=False,
                        ms=5., linewidth=5.,
                        xlabel=None, ylabel=None,
                        ylim=None, yticks=None, xlim=None, xticks=None, xtick_label=None, ytick_label=None,
                        legend_font=12., legend_loc='upper right'):
    plt.figure()
    plt.title(title)
    plt.xlabel(xlabel, fontsize=24); plt.ylabel(ylabel, fontsize=24)
    if xlim or xticks: plt.xticks(xticks, xtick_label, fontsize=24); plt.xlim(xlim)
    if ylim or yticks: plt.yticks(yticks, ytick_label, fontsize=24); plt.ylim(ylim);
    plt.grid(linestyle='dotted',axis='y')

    for idx, line in enumerate(lines):
        if not logy:
            plt.plot(np.arange(len(line)), line, shapes[idx], color=colors[idx], label=labels[idx],
                 linewidth=linewidth, marker=markers[idx], ms=ms)
        else:
            plt.semilogy(np.arange(len(line)), line, shapes[idx], color=colors[idx], label=labels[idx],
                 linewidth=linewidth, marker=markers[idx], ms=ms)

    plt.legend(loc=legend_loc, fontsize=legend_font)
    plt.savefig(save_path, dpi=900, bbox_inches='tight', pad_inches=0)

def plot_learning_curve_uneven(lines, shapes, colors, labels, markers,
                        save_path, title='', logy=False,
                        ms=5., linewidth=5.,
                        xlabel=None, ylabel=None,
                        ylim=None, yticks=None, xlim=None, xticks=None, xtick_label=None, ytick_label=None,
                        legend_font=12., legend_loc='upper right'):
    plt.figure()
    plt.title(title)
    plt.xlabel(xlabel, fontsize=24); plt.ylabel(ylabel, fontsize=24)
    if xlim or xticks: plt.xticks(xticks, xtick_label, fontsize=24); plt.xlim(xlim)
    if ylim or yticks: plt.yticks(yticks, ytick_label, fontsize=24); plt.ylim(ylim);
    plt.grid(linestyle='dotted',axis='y')

    for idx, line in enumerate(lines):
        if not logy:
            plt.plot(xticks, line, shapes[idx], color=colors[idx], label=labels[idx],
                     linewidth=linewidth, marker=markers[idx], ms=ms)
        else:
            plt.semilogy(xticks, line, shapes[idx], color=colors[idx], label=labels[idx],
                         linewidth=linewidth, marker=markers[idx], ms=ms)

    plt.legend(loc=legend_loc, fontsize=legend_font)
    plt.savefig(save_path, dpi=900, bbox_inches='tight', pad_inches=0)

# vary m
if True:
    basic_solution = np.array([822689.788, 1330343.275, 2019677.863, 3768041.58, 6168518.202, 9092962.537])/(60000)
    basic_solution_pp = np.array([672001.545, 981328.654, 1405888.996, 2310591.49, 3598752.027, 4914150.694])/(60000)
    enhanced_solution = np.array([8759830.066, 11632274.13, 14547773.78, 20880090.19, 28016917.77, 35933279.48])/(60000)
    enhanced_solution_pp = np.array([4976876.39, 5775811.063, 6812708.759, 8636597.903, 11084675.21, 13384859.64])/(60000)

    lines = [basic_solution, basic_solution_pp, enhanced_solution, enhanced_solution_pp]
    shapes = ['-', '-.', '-', '-.']
    markers = ['s', 'o', 'D', '^']
    labels = ['Pivot-Basic', 'Pivot-Basic-PP', 'Pivot-Enhanced', 'Pivot-Enhanced-PP']
    #colors = ['#009933', '#66ff33', '#0033cc', '#0066ff']
    colors = ['b', 'g', 'r', 'm']
    xlim = [1.8, 10.2]
    ylim = [0, 800]
    #xticks = range(5)
    xticks = [2, 3, 4, 6, 8, 10]
    yticks = [0, 200, 400, 600, 800]
    xtick_label = ['2', '3', '4', '6', '8', '10']
    ytick_label = ['0', '200', '400', '600', '800']

    plot_learning_curve_uneven(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_m.pdf',
                        title='', logy=False, ms=12, linewidth=3,
                        xlabel='$m$', ylabel='Training Time (min)', ylim=ylim, yticks=yticks,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, ytick_label=ytick_label, legend_font=16, legend_loc='upper left')

# vary n
if False:
    basic_solution = np.array([1160786.092, 1176500.369, 1330343.275, 1587529.102, 2076230.819])/(60000)
    basic_solution_pp = np.array([764980.12, 782337.539, 981328.654, 1222573.492, 1711838.596])/(60000)
    enhanced_solution = np.array([2182740.249, 3214100.898, 11632274.13, 22114537.05, 42753624.01])/(60000)
    enhanced_solution_pp = np.array([1264843.094, 1771361.828, 5775811.063, 10815403.56, 20837979.56])/(60000)

    lines = [basic_solution, basic_solution_pp, enhanced_solution, enhanced_solution_pp]
    shapes = ['-', '-.', '-', '-.']
    markers = ['s', 'o', 'D', '^']
    labels = ['Pivot-Basic', 'Pivot-Basic-PP', 'Pivot-Enhanced', 'Pivot-Enhanced-PP']
    #colors = ['#009933', '#66ff33', '#0033cc', '#0066ff']
    colors = ['b', 'g', 'r', 'm']
    xlim = [-0.2, 4.2]
    ylim = [0, 800]
    xticks = range(5)
    yticks = [0, 200, 400, 600, 800]
    xtick_label = ['5k', '10k', '50k', '100k', '200k']
    ytick_label = ['0', '200', '400', '600', '800']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_n.pdf',
                        title='', logy=False, ms=12, linewidth=3,
                        xlabel='$n$', ylabel='Training Time (min)', ylim=ylim, yticks=yticks,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, ytick_label=ytick_label, legend_font=16, legend_loc='upper left')

# vary d
if False:
    basic_solution = np.array([508702.922, 1330343.275, 2523424.864, 4929702.87, 9991561.719])/(60000)
    basic_solution_pp = np.array([409336.699, 981328.654, 1880478.914, 3583171.214, 6827141.886])/(60000)
    enhanced_solution = np.array([10747758.66, 11632274.13, 12871815.88, 15363180.18, 20376077.21])/(60000)
    enhanced_solution_pp = np.array([5208559.656, 5775811.063, 6667274.732, 8457606.715, 11900533.43])/(60000)

    lines = [basic_solution, basic_solution_pp, enhanced_solution, enhanced_solution_pp]
    shapes = ['-', '-.', '-', '-.']
    markers = ['s', 'o', 'D', '^']
    labels = ['Pivot-Basic', 'Pivot-Basic-PP', 'Pivot-Enhanced', 'Pivot-Enhanced-PP']
    colors = ['b', 'g', 'r', 'm']
    xlim = [-0.2, 4.2]
    ylim = [0, 400]
    xticks = range(5)
    yticks = [0, 100, 200, 300, 400]
    xtick_label = ['5', '15', '30', '60', '120']
    ytick_label = ['0', '100', '200', '300', '400']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_d.pdf',
                        title='', logy=False, ms=12, linewidth=3,
                        xlabel='$\\bar{d}$', ylabel='Training Time (min)', ylim=ylim, yticks=yticks,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, ytick_label=ytick_label, legend_font=16, legend_loc='upper left')

# vary split_num
if False:
    basic_solution = np.array([374593.067, 690494.683, 1330343.275, 2586320.03, 5018416.149])/(60000)
    basic_solution_pp = np.array([335274.584, 558495.617, 981328.654, 1826438.97, 3560476.637])/(60000)
    enhanced_solution = np.array([10567803.76, 10901940.05, 11632274.13, 12768027.69, 15257682.35])/(60000)
    enhanced_solution_pp = np.array([5135410.606, 5351739.312, 5775811.063, 6668996.846, 8397098.167])/(60000)

    lines = [basic_solution, basic_solution_pp, enhanced_solution, enhanced_solution_pp]
    shapes = ['-', '-.', '-', '-.']
    markers = ['s', 'o', 'D', '^']
    labels = ['Pivot-Basic', 'Pivot-Basic-PP', 'Pivot-Enhanced', 'Pivot-Enhanced-PP']
    colors = ['b', 'g', 'r', 'm']
    xlim = [-0.2, 4.2]
    ylim = [0, 300]
    xticks = range(5)
    yticks = [0, 100, 200, 300]
    xtick_label = ['2', '4', '8', '16', '32']
    ytick_label = ['0', '100', '200', '300']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_splitNum.pdf',
                        title='', logy=False, ms=12, linewidth=3,
                        xlabel='$\\beta$', ylabel='Training Time (min)', ylim=ylim, yticks=yticks,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, ytick_label=ytick_label, legend_font=16, legend_loc='upper left')

# vary tree_depth
if False:
    basic_solution = np.array([352731.551, 694191.652, 1330343.275, 2634886.799, 5134124.694])/(60000)
    basic_solution_pp = np.array([277186.347, 538449.966, 981328.654, 1973280.671, 3832426.848])/(60000)
    enhanced_solution = np.array([2389211.304, 5487717.84, 11632274.13, 23730182.92, 48868967.21])/(60000)
    enhanced_solution_pp = np.array([1246909.41, 2776216.228, 5775811.063, 11928529.71, 24082958.42])/(60000)

    lines = [basic_solution, basic_solution_pp, enhanced_solution, enhanced_solution_pp]
    shapes = ['-', '-.', '-', '-.']
    markers = ['s', 'o', 'D', '^']
    labels = ['Pivot-Basic', 'Pivot-Basic-PP', 'Pivot-Enhanced', 'Pivot-Enhanced-PP']
    colors = ['b', 'g', 'r', 'm']
    xlim = [-0.2, 4.2]
    ylim = [0, 850]
    xticks = range(5)
    yticks = [0, 200, 400, 600, 800]
    xtick_label = ['2', '3', '4', '5', '6']
    ytick_label = ['0', '200', '400', '600', '800']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_treeDepth.pdf',
                        title='', logy=False, ms=12, linewidth=3,
                        xlabel='$h$', ylabel='Training Time (min)', ylim=ylim, yticks=yticks,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, ytick_label=ytick_label, legend_font=16, legend_loc='upper left')

# vary numTree
if False:
    basic_solution_RF_PP_Classification = np.array([969793.061, 1944277.87, 3957691.258, 7855930.299, 15662272.89]) / (60000*60)
    #basic_solution_GBDT_PP_Classification = np.array([70764000.23, 141634146.5, 282783904.1, 566458577.6, 1132917155]) / (60000*60)
    basic_solution_GBDT_PP_Classification = np.array([15614170.854, 37528341.708, 81356683.416, 169013366.832, 344326733.664]) / (60000*60)
    basic_solution_RF_PP_Regression = np.array([938691.05, 1911267.56, 3827353.518, 7655725.31, 15218943.76]) / (60000*60)
    #basic_solution_GBDT_PP_Regression = np.array([17237523.9, 35336923.99, 70673847.98, 141347696, 282695391.9]) / (60000*60)
    basic_solution_GBDT_PP_Regression = np.array([2331123.216, 4662246.432, 9324492.864, 18648985.728, 37297971.456]) / (60000*60)

    lines = [basic_solution_RF_PP_Classification, basic_solution_GBDT_PP_Classification, basic_solution_RF_PP_Regression, basic_solution_GBDT_PP_Regression]
    #lines = [basic_solution_RF_PP_Classification, basic_solution_RF_PP_Regression, basic_solution_GBDT_PP_Regression]
    shapes = ['-', '-', '-', '-']
    markers = ['s', 'o', 'D', '^']
    labels = ['Pivot-RF-Classification', 'Pivot-GBDT-Classification', 'Pivot-RF-Regression', 'Pivot-GBDT-Regression']
    colors = ['b', 'g', 'r', 'm']
    xlim = [-0.2, 4.2]
    ylim = [0, 100]
    xticks = range(5)
    yticks = [0, 20, 40, 60, 80, 100]
    xtick_label = ['2', '4', '8', '16', '32']
    ytick_label = ['0', '20', '40', '60', '80', '100']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_numTree.pdf',
                        title='', logy=False, ms=12, linewidth=3,
                        xlabel='$K$', ylabel='Training Time (hour)', ylim=ylim, yticks=yticks,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, ytick_label=ytick_label, legend_font=16, legend_loc='upper left')

# vary m_2
if True:
    basic_solution = np.array([9.884, 14.132, 19.003, 27.878, 37.125, 46.541])
    enhanced_solution = np.array([37.624, 38.978, 39.862, 41.612, 45.503, 47.937])

    lines = [basic_solution, enhanced_solution]
    shapes = ['-', '-']
    markers = ['s', 'D']
    labels = ['Pivot-Basic', 'Pivot-Enhanced']
    colors = ['b', 'r']
    xlim = [1.8, 10.2]
    ylim = [0, 60]
    #xticks = range(5)
    xticks = [2, 3, 4, 6, 8, 10]
    yticks = [0, 20, 40, 60]
    xtick_label = ['2', '3', '4', '6', '8', '10']
    ytick_label = ['0', '20', '40', '60']

    plot_learning_curve_uneven(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_m_prediction.pdf',
                        title='', logy=False, ms=12, linewidth=3,
                        xlabel='$m$', ylabel='Prediction Time (ms)', ylim=ylim, yticks=yticks,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, ytick_label=ytick_label, legend_font=16, legend_loc='upper left')

# vary tree_depth_2
if False:
    basic_solution = np.array([13.896, 13.963, 14.132, 14.437, 15.211])
    enhanced_solution = np.array([8.022, 17.908, 38.978, 76.721, 158.525])

    lines = [basic_solution, enhanced_solution]
    shapes = ['-', '-']
    markers = ['s', 'D']
    labels = ['Pivot-Basic', 'Pivot-Enhanced']
    colors = ['b', 'r']
    xlim = [-0.2, 4.2]
    ylim = [0, 165]
    xticks = range(5)
    yticks = [0, 40, 80, 120, 160]
    xtick_label = ['2', '3', '4', '5', '6']
    ytick_label = ['0', '40', '80', '120', '160']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_treeDepth_prediction.pdf',
                        title='', logy=False, ms=12, linewidth=3,
                        xlabel='$h$', ylabel='Prediction Time (ms)', ylim=ylim, yticks=yticks,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, ytick_label=ytick_label, legend_font=16, legend_loc='upper left')

# vary num_tree_2
if False:
    basic_solution_RF_PP = np.array([29.177, 57.602, 116.529, 237.042, 461.656])
    basic_solution_GBDT_PP = np.array([116.208, 232.947, 461.657, 923.313, 1846.628])

    lines = [basic_solution_RF_PP, basic_solution_GBDT_PP]
    shapes = ['-', '-.']
    markers = ['v', '^']
    labels = ['Pivot-Basic-RF-PP', 'Pivot-Basic-GBDT-PP']
    colors = ['#0033cc', '#0066ff']
    xlim = [-0.2, 4.2]
    ylim = None
    xticks = range(5)
    xtick_label = ['2', '4', '8', '16', '32']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_numTree_2.pdf',
                        title='', logy=False, ms=3, linewidth=0.8,
                        xlabel='Tree Number', ylabel='Training Time (ms)', ylim=ylim, yticks=None,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, legend_font=8, legend_loc='upper left')

# vary m_3
if True:
    basic_solution = np.array([822689.788, 1330343.275, 2019677.863, 3768041.58, 6168518.202, 9092962.537])/(60000)
    enhanced_solution = np.array([8759830.066, 11632274.13, 14547773.78, 20880090.19, 28016917.77, 35933279.48])/(60000)
    SPDZ_DZ = np.array([8307152.885, 19876233.69, 35191066.55, 74834308.21, 115319597.9, 168851399.6])/(60000)
    Non_private = np.array([3041, 4666, 6231, 9548, 12674, 15591])/(60000)

    lines = [basic_solution, enhanced_solution, SPDZ_DZ, Non_private]
    shapes = ['-', '-', '-', '-']
    markers = ['s', 'D', 'P', 'v']
    labels = ['Pivot-Basic', 'Pivot-Enhanced', 'SPDZ-DT', 'NP-DT']
    colors = ['b', 'r', 'c', 'm']
    xlim = [1.8, 10.2]
    ylim = [0, 3000]
    #xticks = range(6)
    xticks = [2, 3, 4, 6, 8, 10]
    yticks = [0, 1000, 2000, 3000]
    xtick_label = ['2', '3', '4', '6','8', '10']
    ytick_label = ['0', '1000', '2000', '3000']

    plot_learning_curve_uneven(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_m_comparison.pdf',
                        title='', logy=False, ms=12, linewidth=3,
                        xlabel='$m$', ylabel='Training Time (min)', ylim=ylim, yticks=yticks,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, ytick_label=ytick_label, legend_font=16, legend_loc='upper left')

# vary n_2
if True:
    basic_solution = np.array([1160786.092, 1176500.369, 1330343.275, 1587529.102, 2076230.819])/(60000)
    enhanced_solution = np.array([2182740.249, 3214100.898, 11632274.13, 22114537.05, 42753624.01])/(60000)
    SPDZ_DZ = np.array([1950356.105, 5102164.833, 19876233.69, 57118889.29, 77949970.76])/(60000)
    non_private = np.array([1006, 1363, 4666, 8797, 17563])/(60000)

    lines = [basic_solution, enhanced_solution, SPDZ_DZ, non_private]
    shapes = ['-', '-', '-', '-']
    markers = ['s', 'D', 'P', 'v']
    labels = ['Pivot-Basic', 'Pivot-Enhanced', 'SPDZ-DT', 'NP-DT']
    colors = ['b', 'r', 'c', 'm']
    xlim = [-0.2, 4.2]
    ylim = [0, 1500]
    xticks = range(5)
    yticks = [0, 300, 600, 900, 1200, 1500]
    xtick_label = ['5k', '10k', '50k', '100k', '200k']
    ytick_label = ['0', '300', '600', '900', '1200', '1500']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_n_comparison.pdf',
                        title='', logy=False, ms=12, linewidth=3,
                        xlabel='$n$', ylabel='Training Time (min)', ylim=ylim, yticks=yticks,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, ytick_label=ytick_label, legend_font=16, legend_loc='upper left')





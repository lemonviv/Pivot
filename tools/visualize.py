import numpy as np
import matplotlib.pyplot as plt

def plot_learning_curve(lines, shapes, colors, labels, markers,
                        save_path, title='', logy=False,
                        ms=1., linewidth=1.,
                        xlabel=None, ylabel=None,
                        ylim=None, yticks=None, xlim=None, xticks=None, xtick_label=None,
                        legend_font=6., legend_loc='upper right'):
    plt.figure()
    plt.title(title)
    plt.xlabel(xlabel); plt.ylabel(ylabel)
    if xlim or xticks: plt.xticks(xticks, xtick_label); plt.xlim(xlim)
    if ylim or yticks: plt.yticks(yticks); plt.ylim(ylim);
    # plt.grid(linestyle='dotted')

    for idx, line in enumerate(lines):
        if not logy:
            plt.plot(np.arange(len(line)), line, shapes[idx], color=colors[idx], label=labels[idx],
                 linewidth=linewidth, marker=markers[idx], ms=ms)
        else:
            plt.semilogy(np.arange(len(line)), line, shapes[idx], color=colors[idx], label=labels[idx],
                 linewidth=linewidth, marker=markers[idx], ms=ms)

    plt.legend(loc=legend_loc, fontsize=legend_font)
    plt.savefig(save_path, dpi=900, bbox_inches='tight', pad_inches=0)


# vary m
if False:
    basic_solution = np.array([822689.788, 1330343.275, 2019677.863, 2806841.81, 3768041.58])/(60000)
    basic_solution_pp = np.array([672001.545, 981328.654, 1405888.996, 1789179.203, 2310591.49])/(60000)
    enhanced_solution = np.array([8759830.066, 11632274.13, 14547773.78, 17610923.62, 20880090.19])/(60000)
    enhanced_solution_pp = np.array([4976876.39, 5775811.063, 6812708.759, 7656300.29, 8636597.903])/(60000)

    lines = [basic_solution, basic_solution_pp, enhanced_solution, enhanced_solution_pp]
    shapes = ['-.', '-', '-.', '-']
    markers = ['v', '^', '<', '>']
    labels = ['Basic-Solution', 'Basic-Solution-PP', 'Enhanced-Solution', 'Enhanced-Solution-PP']
    colors = ['#009933', '#66ff33', '#0033cc', '#0066ff']
    xlim = [-0.2, 4.2]
    xticks = range(5)
    xtick_label = ['2', '3', '4', '5', '6']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_m.pdf',
                        title='', logy=False, ms=3, linewidth=0.8,
                        xlabel='m', ylabel='Training Time (min)', ylim=None, yticks=None,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, legend_font=8, legend_loc='upper left')

# vary n
if False:
    basic_solution = np.array([1160786.092, 1176500.369, 1330343.275, 1587529.102, 2076230.819])/(60000)
    basic_solution_pp = np.array([764980.12, 782337.539, 981328.654, 1222573.492, 1711838.596])/(60000)
    enhanced_solution = np.array([2182740.249, 3214100.898, 11632274.13, 22114537.05, 42753624.01])/(60000)
    enhanced_solution_pp = np.array([1264843.094, 1771361.828, 5775811.063, 10815403.56, 20837979.56])/(60000)

    lines = [basic_solution, basic_solution_pp, enhanced_solution, enhanced_solution_pp]
    shapes = ['-.', '-', '-.', '-']
    markers = ['v', '^', '<', '>']
    labels = ['Basic-Solution', 'Basic-Solution-PP', 'Enhanced-Solution', 'Enhanced-Solution-PP']
    colors = ['#009933', '#66ff33', '#0033cc', '#0066ff']
    xlim = [-0.2, 4.2]
    xticks = range(5)
    xtick_label = ['5k', '10k', '50k', '100k', '200k']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_n.pdf',
                        title='', logy=False, ms=3, linewidth=0.8,
                        xlabel='n', ylabel='Training Time (min)', ylim=None, yticks=None,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, legend_font=8, legend_loc='upper left')

# vary d
if False:
    basic_solution = np.array([508702.922, 919301.882, 1330343.275, 1710651.197, 2109158.354])/(60000)
    basic_solution_pp = np.array([409336.699, 695817.828, 981328.654, 1295237.611, 1560928.826])/(60000)
    enhanced_solution = np.array([10747758.66, 11147677.36, 11632274.13, 12117448.09, 12354410.37])/(60000)
    enhanced_solution_pp = np.array([5208559.656, 5463166.901, 5775811.063, 6079214.553, 6347284.226])/(60000)

    lines = [basic_solution, basic_solution_pp, enhanced_solution, enhanced_solution_pp]
    shapes = ['-.', '-', '-.', '-']
    markers = ['v', '^', '<', '>']
    labels = ['Basic-Solution', 'Basic-Solution-PP', 'Enhanced-Solution', 'Enhanced-Solution-PP']
    colors = ['#009933', '#66ff33', '#0033cc', '#0066ff']
    xlim = [-0.2, 4.2]
    ylim = [0, 250]
    xticks = range(5)
    xtick_label = ['5', '10', '15', '20', '25']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_d.pdf',
                        title='', logy=False, ms=3, linewidth=0.8,
                        xlabel='d', ylabel='Training Time (min)', ylim=ylim, yticks=None,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, legend_font=8, legend_loc='upper left')

# vary split_num
if False:
    basic_solution = np.array([374593.067, 690494.683, 1330343.275, 2586320.03, 5018416.149])/(60000)
    basic_solution_pp = np.array([335274.584, 558495.617, 981328.654, 1826438.97, 3560476.637])/(60000)
    enhanced_solution = np.array([10567803.76, 10901940.05, 11632274.13, 12768027.69, 15257682.35])/(60000)
    enhanced_solution_pp = np.array([5135410.606, 5351739.312, 5775811.063, 6668996.846, 8397098.167])/(60000)

    lines = [basic_solution, basic_solution_pp, enhanced_solution, enhanced_solution_pp]
    shapes = ['-.', '-', '-.', '-']
    markers = ['v', '^', '<', '>']
    labels = ['Basic-Solution', 'Basic-Solution-PP', 'Enhanced-Solution', 'Enhanced-Solution-PP']
    colors = ['#009933', '#66ff33', '#0033cc', '#0066ff']
    xlim = [-0.2, 4.2]
    ylim = None
    xticks = range(5)
    xtick_label = ['2', '4', '8', '16', '32']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_splitNum.pdf',
                        title='', logy=False, ms=3, linewidth=0.8,
                        xlabel='Split Number', ylabel='Training Time (min)', ylim=ylim, yticks=None,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, legend_font=8, legend_loc='upper left')

# vary tree_depth
if False:
    basic_solution = np.array([352731.551, 694191.652, 1330343.275, 2634886.799, 5134124.694])/(60000)
    basic_solution_pp = np.array([277186.347, 538449.966, 981328.654, 1973280.671, 3832426.848])/(60000)
    enhanced_solution = np.array([2389211.304, 5487717.84, 11632274.13, 23730182.92, 48868967.21])/(60000)
    enhanced_solution_pp = np.array([1246909.41, 2776216.228, 5775811.063, 11928529.71, 24082958.42])/(60000)

    lines = [basic_solution, basic_solution_pp, enhanced_solution, enhanced_solution_pp]
    shapes = ['-.', '-', '-.', '-']
    markers = ['v', '^', '<', '>']
    labels = ['Basic-Solution', 'Basic-Solution-PP', 'Enhanced-Solution', 'Enhanced-Solution-PP']
    colors = ['#009933', '#66ff33', '#0033cc', '#0066ff']
    xlim = [-0.2, 4.2]
    ylim = None
    xticks = range(5)
    xtick_label = ['2', '3', '4', '5', '6']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_treeDepth.pdf',
                        title='', logy=False, ms=3, linewidth=0.8,
                        xlabel='Tree Depth', ylabel='Training Time (min)', ylim=ylim, yticks=None,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, legend_font=8, legend_loc='upper left')

# vary tree_depth
if False:
    basic_solution_RF_PP = np.array([969793.061, 1944277.87, 3957691.258, 7855930.299, 15662272.89]) / (60000)
    basic_solution_GBDT_PP = np.array([21324089.88, 42678588.06, 85364293.49, 170728587, 341457173]) / (60000)

    lines = [basic_solution_RF_PP, basic_solution_GBDT_PP]
    shapes = ['-', '-.']
    markers = ['v', '^']
    labels = ['Basic-Solution-RF-PP', 'Basic-Solution-GBDT-PP']
    colors = ['#0033cc', '#0066ff']
    xlim = [-0.2, 4.2]
    ylim = None
    xticks = range(5)
    xtick_label = ['2', '4', '8', '16', '32']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_numTree.pdf',
                        title='', logy=False, ms=3, linewidth=0.8,
                        xlabel='Tree Depth', ylabel='Training Time (min)', ylim=ylim, yticks=None,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, legend_font=8, legend_loc='upper left')

# vary m_2
if False:
    basic_solution = np.array([9.884, 14.132, 19.003, 23.653, 27.878])
    enhanced_solution = np.array([37.624, 38.978, 39.862, 40.85, 41.612])

    lines = [basic_solution, enhanced_solution]
    shapes = ['-', '-.']
    markers = ['v', '^']
    labels = ['Basic-Solution', 'Enhanced-Solution']
    colors = ['#0033cc', '#0066ff']
    xlim = [-0.2, 4.2]
    ylim = [8, 48]
    xticks = range(5)
    xtick_label = ['2', '3', '4', '5', '6']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_m_2.pdf',
                        title='', logy=False, ms=3, linewidth=0.8,
                        xlabel='m', ylabel='Training Time (ms)', ylim=ylim, yticks=None,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, legend_font=8, legend_loc='upper left')

# vary tree_depth_2
if False:
    basic_solution = np.array([13.896, 13.963, 14.132, 14.437, 15.211])
    enhanced_solution = np.array([8.022, 17.908, 38.978, 76.721, 158.525])

    lines = [basic_solution, enhanced_solution]
    shapes = ['-', '-.']
    markers = ['v', '^']
    labels = ['Basic-Solution', 'Enhanced-Solution']
    colors = ['#0033cc', '#0066ff']
    xlim = [-0.2, 4.2]
    ylim = None
    xticks = range(5)
    xtick_label = ['2', '3', '4', '5', '6']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_treeDepth_2.pdf',
                        title='', logy=False, ms=3, linewidth=0.8,
                        xlabel='Tree Depth', ylabel='Training Time (ms)', ylim=ylim, yticks=None,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, legend_font=8, legend_loc='upper left')

# vary num_tree_2
if False:
    basic_solution_RF_PP = np.array([29.177, 57.602, 116.529, 237.042, 461.656])
    basic_solution_GBDT_PP = np.array([116.208, 232.947, 461.657, 923.313, 1846.628])

    lines = [basic_solution_RF_PP, basic_solution_GBDT_PP]
    shapes = ['-', '-.']
    markers = ['v', '^']
    labels = ['Basic-Solution-RF-PP', 'Basic-Solution-GBDT-PP']
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
if False:
    basic_solution = np.array([822689.788, 1330343.275, 2019677.863, 2806841.81, 3768041.58])/(60000)
    enhanced_solution = np.array([8759830.066, 11632274.13, 14547773.78, 17610923.62, 20880090.19])/(60000)
    SPDZ_DZ = np.array([8307152.885, 19876233.69, 35191066.55, 53773722.08, 74834308.21])/(60000)

    lines = [basic_solution, enhanced_solution, SPDZ_DZ]
    shapes = ['-', '-.', ':']
    markers = ['v', '^', '>']
    labels = ['Basic-Solution', 'Enhanced-Solution', 'SPDZ-DT']
    colors = ['#0033cc', '#0066ff', '#6666ff']
    xlim = [-0.2, 4.2]
    ylim = None
    xticks = range(5)
    xtick_label = ['2', '3', '4', '5', '6']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_m_3.pdf',
                        title='', logy=False, ms=3, linewidth=0.8,
                        xlabel='m', ylabel='Training Time (min)', ylim=ylim, yticks=None,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, legend_font=8, legend_loc='upper left')

# vary n_2
if True:
    basic_solution = np.array([1160786.092, 1176500.369, 1330343.275, 1587529.102, 2076230.819])/(60000)
    enhanced_solution = np.array([2182740.249, 3214100.898, 11632274.13, 22114537.05, 42753624.01])/(60000)
    SPDZ_DZ = np.array([1950356.105, 5102164.833, 19876233.69, 57118889.29, 77949970.76])/(60000)

    lines = [basic_solution, enhanced_solution, SPDZ_DZ]
    shapes = ['-', '-.', ':']
    markers = ['v', '^', '>']
    labels = ['Basic-Solution', 'Enhanced-Solution', 'SPDZ-DT']
    colors = ['#0033cc', '#0066ff', '#6666ff']
    xlim = [-0.2, 4.2]
    ylim = None
    xticks = range(5)
    xtick_label = ['5k', '25k', '50k', '100k', '200k']

    plot_learning_curve(lines=lines, shapes=shapes, colors=colors, labels=labels, markers=markers,
                        save_path='figs/vary_n_2.pdf',
                        title='', logy=False, ms=3, linewidth=0.8,
                        xlabel='n', ylabel='Training Time (min)', ylim=ylim, yticks=None,
                        xlim=xlim, xticks=xticks, xtick_label=xtick_label, legend_font=8, legend_loc='upper left')




